#include <nan_object_wrap_template.h>
#include <iostream>
#include <sstream>

#if NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION
#include <tr1/unordered_map>
#else
#include <map>
#endif

// made with *some* help from:
// - https://github.com/brodybits/nan/blob/cb-object-wrapper-template/test/cpp/wrappedobjectfactory.cpp
//   which is based on:
//   https://github.com/nodejs/nan/blob/master/test/cpp/objectwraphandle.cpp

class HTTPServer;

struct PathInfo {
  PathInfo(v8::Local<v8::Object> srvHandle, const char * path, int code, const char * content) :
    ps(srvHandle), isRoute(true), path(path), code(code), content(content), hasCB(false) {}

  // XXX TODO: should store & use V8 isolate value:
  PathInfo(v8::Local<v8::Object> srvHandle, v8::Local<v8::Function> & f) :
    ps(srvHandle), isRoute(true), pf(f), hasCB(true) {}

  PathInfo(const PathInfo & rhs) : ps(Nan::New(rhs.ps)),
    isRoute(rhs.isRoute), hasCB(rhs.hasCB),
    path(rhs.path), code(rhs.code),
    content(rhs.content), pf(Nan::New(rhs.pf)) {}

  PathInfo() : isRoute(false), hasCB(false), code(404) {}

  PathInfo & operator=(PathInfo & rhs) {
    ps.Reset(Nan::New(rhs.ps));
    isRoute = rhs.isRoute;
    hasCB = rhs.hasCB;
    path = rhs.path;
    code = rhs.code;
    content = rhs.content;
    pf.Reset(Nan::New(rhs.pf));
    return *this;
  }

  // NOTE: keep persistent handle of the HTTPServer object just the
  // V8 Javascript side may release it:
  Nan::Persistent<v8::Object> ps;

  bool isRoute;
  bool hasCB;

  std::string path;
  int code;
  std::string content;

  Nan::Persistent<v8::Function> pf;
};

class HTTPServerReq : public ObjectWrapTemplateBase<HTTPServerReq> {
public:
  static inline v8::Local<v8::FunctionTemplate>
  MakeConstructorFunctionTemplate(const char *object_class_name) {
    v8::Local<v8::FunctionTemplate> tpl =
      NewConstructorFunctionTemplate(object_class_name, 1);
    SetPrototypeMethod(tpl, "res", res);
    return tpl;
  }

  HTTPServerReq(Nan::NAN_METHOD_ARGS_TYPE args_info) : info(NULL) {} //, r(NULL) {}

  ~HTTPServerReq() {}

  static v8::Local<v8::Value> NewInstance(v8::Local<v8::Function> cons, int argc, v8::Local<v8::Value> argv[]) {
    return NewInstanceMethod(cons, argc, argv);
  }

  static void res(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    HTTPServerReq * myself = ObjectFromMethodArgsInfo(args_info);

    if (args_info.Length() < 2 ||
        !args_info[0]->IsInt32() ||
        !args_info[1]->IsString()) {
      std::cerr << "Sorry incorrect arguments to response function" << std::endl;
      return;
    }

    std::string cs (*v8::String::Utf8Value(args_info[1]->ToString()));

    //uv_write_t mywrite;
    uv_write_t * writehandle = new uv_write_t;
    writehandle->data = myself;

    std::stringstream sst;
    // XXX TODO FIX RESPONSE CODE
    sst << "HTTP/1.1 200 OK\r\nContent-Length: " << cs.length() <<
        "\r\n\r\n" << cs;
    std::string resp(sst.str());
    uv_buf_t mybuf;
    mybuf.base = (char *)(resp.c_str());
    mybuf.len = resp.length();
    uv_write(writehandle, myself->s, &mybuf, 1, writecb);
  }

  static void writecb(uv_write_t * w, int) {
    HTTPServerReq * myself = reinterpret_cast<HTTPServerReq *>(w->data);

    delete w;

    uv_close(reinterpret_cast<uv_handle_t *>(myself->s), closecb);
  }

  static void closecb(uv_handle_t * h) {
    delete h;
  }

  PathInfo * info;

  uv_stream_t * s;
};

class HTTPServer : public ObjectWrapTemplateBase<HTTPServer> {
public:
  static inline v8::Local<v8::FunctionTemplate>
  MakeConstructorFunctionTemplate(const char *object_class_name) {
    v8::Local<v8::FunctionTemplate> tpl =
      NewConstructorFunctionTemplate(object_class_name, 1);
    SetPrototypeMethod(tpl, "staticPath", StaticPath);
    SetPrototypeMethod(tpl, "pathCB", PathCB);
    SetPrototypeMethod(tpl, "bindSocket", BindSocket);
    SetPrototypeMethod(tpl, "bindAddr", BindAddr);
    return tpl;
  }

  HTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    static int my_class_id = 0;

    std::stringstream sst;
    sst << "HTTPServerReq_" << my_class_id;

    req_constructor_tpl.Reset(
      HTTPServerReq::MakeConstructorFunctionTemplate(
        sst.str().c_str()));

    req_constructor.Reset(
      Nan::GetFunction(Nan::New(req_constructor_tpl)).ToLocalChecked());
  }

  ~HTTPServer() {}

  static void NewInstance(v8::Local<v8::Function> cons, Nan::NAN_METHOD_ARGS_TYPE args_info) {
    NewInstanceMethod(cons, args_info);
  }

  static void StaticPath(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    HTTPServer * myself = ObjectFromMethodArgsInfo(args_info);

    if (args_info.Length() < 3 ||
        !args_info[0]->IsString() ||
        !args_info[1]->IsInt32() ||
        !args_info[2]->IsString()) {
      std::cerr << "Sorry incorrect arguments to staticPath" << std::endl;
      return;
    }

    std::string mypath(*v8::String::Utf8Value(args_info[0]->ToString()));

    // XXX TBD ???:
    if (mypath[0] != '/') {
      std::cerr << "Sorry invalid path" << std::endl;
      return;
    }

    // XXX TODO:
    // - Support true Buffer(s)
    // - TBD ???: free the memory in case this static path is no longer relevant
    // - TBD ???: get rid of extra std::string storage to make this more efficient
    std::string mycontent(*v8::String::Utf8Value(args_info[2]->ToString()));

    PathInfo pi(myself->handle(), mypath.c_str(), args_info[1]->Int32Value(), mycontent.c_str());
    myself->routes[mypath] = pi;
  }

  // XXX FUTURE TBD: function to remove a path

  static void PathCB(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    HTTPServer * myself = ObjectFromMethodArgsInfo(args_info);

    if (args_info.Length() < 2 ||
        !args_info[0]->IsString() ||
        !args_info[1]->IsFunction()) {
      std::cerr << "Sorry incorrect arguments to pathCB()" << std::endl;
      return;
    }

    std::string mypath(*v8::String::Utf8Value(args_info[0]->ToString()));

    // XXX TBD ???:
    if (mypath[0] != '/') {
      std::cerr << "Sorry invalid path" << std::endl;
      return;
    }

    // XXX TODO:
    // - Support true Buffer(s)
    // - TBD ???: get rid of extra std::string storage to make this more efficient
    v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(args_info[1]);

    PathInfo pi(myself->handle(), f);
    myself->routes[mypath] = pi;
  }

#if NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION
  static void AllocForRead(uv_handle_t *, size_t s, uv_buf_t * b) {
    void * mybuf = malloc(s);
    b->base = reinterpret_cast<char *>(mybuf);
    b->len = s;
  }
#else
  static uv_buf_t AllocForRead(uv_handle_t *, size_t s) {
    void * mybuf = malloc(s);
    uv_buf_t b = {
      .base = reinterpret_cast<char *>(mybuf),
      .len = s
    };
    return b;
  }
#endif

#if NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION
  static void HandleReadCB(uv_stream_t * s, ssize_t n, const uv_buf_t * b)
#else
static void HandleReadCB(uv_stream_t * s, ssize_t n, uv_buf_t buf)
#endif
  {
#if NODE_MODULE_VERSION <= NODE_0_10_MODULE_VERSION
    const uv_buf_t * b = &buf;
#endif
    std::cout << "read cb n: " << n << std::endl;

    // XXX TODO [MISSING]:
    // - must wait for entire HTTP request
    // - error checking
    // - Keep-Alive support
    // (use @nodejs/http-parser to [help] solve these)

    if (n < 0) return;

    HTTPServer * myself = reinterpret_cast<HTTPServer *>(s->data);

    std::string url;

    if (n > 0) {
      std::string s((const char *)b->base, n);
      std::cout << "read: " << s << std::endl;
      if (s.length() >= 5 && s.substr(0, 5) == "GET /") {
        //int p2 = s.find(4, '/');
        int p2 = 4;
        while (p2 < n && s[p2] != ' ') ++p2;
        std::cout << "p2: " << p2 << std::endl;
        if (p2 != std::string::npos) {
          //url = s.substr(4, p2);
          url = s.substr(4, p2-4);
          std::cout << "Got url: " << url << std::endl;
        }
      }
    }

    auto found = myself->routes.find(url);

    // XXX UGLY (TODO CLEANUP)
    if (found != myself->routes.end() && found->second.isRoute) {
      //std::cout << "found" << std::endl;
      if (found->second.hasCB) {
        Nan::HandleScope myscope;

        Nan::Callback cb(Nan::New(found->second.pf));

        // XXX UGLY (TODO CLEANUP)
        v8::Local<v8::Value> sr_argv[1] = {Nan::New(1)};
        v8::Local<v8::Object> sr = HTTPServerReq::NewInstance(
          Nan::New(myself->req_constructor), 1, sr_argv)->ToObject();

        HTTPServerReq * mysr = ObjectWrap::Unwrap<HTTPServerReq>(sr);
        // XXX VERY UGLY
        mysr->info = &found->second;
        mysr->s = s;

        static int argc = 1;
        v8::Local<v8::Value> argv[1] = {sr};

        cb.Call(argc, argv);
      } else {
        // XXX UGLY (TODO CLEANUP)
        std::cout << "found static" << std::endl;
        std::cout << "with code: " << found->second.code << std::endl;
        //uv_write_t mywrite;
        uv_write_t * writehandle = new uv_write_t;
        writehandle->data = s;
        //std::string resp("HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nabc\n");
        std::stringstream sst;
        // XXX TODO FIX RESPONSE CODE
        sst << "HTTP/1.1 200 OK\r\nContent-Length: " << found->second.content.length() <<
            "\r\n\r\n" << found->second.content;
        std::string resp(sst.str());
        uv_buf_t mybuf;
        mybuf.base = (char *)(resp.c_str());
        mybuf.len = resp.length();
        uv_write(writehandle, s, &mybuf, 1, writecb);
      }
    } else {
      uv_write_t mywrite;
      std::string resp("HTTP/1.1 404 Not Found\r\nContent-Length: 16\r\n\r\nSorry not found\n");
      uv_buf_t mybuf;
      mybuf.base = (char *)(resp.c_str());
      mybuf.len = resp.length();
      uv_write(&mywrite, s, &mybuf, 1, NULL);
    }

    free(b->base);
  }

  static void writecb(uv_write_t * w, int) {
    uv_close(reinterpret_cast<uv_handle_t *>(w->data), closecb);
    delete w;
  }

  static void closecb(uv_handle_t * h) {
    delete h;
  }

  static void HandleNewConnection(uv_stream_t *s, int st) {
    if (st < 0) {
      std::cerr << "new connection with error status: " << st << std::endl;
      return;
    }

    uv_tcp_t * tcpin = new uv_tcp_t;

    uv_tcp_init(uv_default_loop(), tcpin);
    tcpin->data = s->data;

    int r = uv_accept(s, (uv_stream_t *)tcpin);
    if (r != 0) {
      uv_close(reinterpret_cast<uv_handle_t *>(tcpin), NULL);
      return;
    }
    uv_read_start(reinterpret_cast<uv_stream_t *>(tcpin), AllocForRead, HandleReadCB);
  }

  static void BindSocket(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    HTTPServer * myself = ObjectFromMethodArgsInfo(args_info);

    if (args_info.Length() < 3 ||
        !args_info[0]->IsString() ||
        !args_info[1]->IsInt32() ||
        !args_info[2]->IsInt32()) {
      std::cerr << "Sorry incorrect arguments to bindSocket" << std::endl;
      return;
    }
  }

  static void BindAddr(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    HTTPServer * myself = ObjectFromMethodArgsInfo(args_info);

    if (args_info.Length() < 3 ||
        !args_info[0]->IsString() ||
        !args_info[1]->IsInt32()) {
      std::cerr << "Sorry incorrect arguments to bindAddr" << std::endl;
      return;
    }

    // *some* help from: https://nikhilm.github.io/uvbook/networking.html
    struct sockaddr_in myaddr;

    uv_tcp_t * mytcp = &myself->mytcphandle;

    uv_tcp_init(uv_default_loop(), mytcp);
    mytcp->data = myself;

    // XXX TODO TODO IP 6:
#if NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION
    uv_ip4_addr(*v8::String::Utf8Value(args_info[0]->ToString()),
                args_info[1]->Int32Value(), &myaddr);
    uv_tcp_bind(mytcp, reinterpret_cast<const sockaddr *>(&myaddr), 0);
#else
    myaddr = uv_ip4_addr(*v8::String::Utf8Value(args_info[0]->ToString()),
                         args_info[1]->Int32Value());
    uv_tcp_bind(mytcp, myaddr);
#endif

    int res = uv_listen(reinterpret_cast<uv_stream_t *>(mytcp), 1024, HandleNewConnection);

    if (res != 0) {
      std::cerr << "sorry listen error: " << res << std::endl;
    }
  }

  // XXX TODO support closing

  // XXX TODO use something better such as @c9s/r3 instead (!)
#if NODE_MODULE_VERSION > NODE_0_10_MODULE_VERSION
  std::tr1::unordered_map<std::string, PathInfo> routes;
#else
  std::map<std::string, PathInfo> routes;
#endif

  // FUTURE TBD ???: mTCP - user-space TCP
  uv_tcp_t mytcphandle;

  Nan::Persistent<v8::FunctionTemplate> req_constructor_tpl;
  Nan::Persistent<v8::Function> req_constructor;
};

static void NewHTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
  static int my_class_id = 0;

  std::stringstream sst;
  sst << "HTTPServer_" << my_class_id;

  HTTPServer::NewInstance(Nan::GetFunction(
    HTTPServer::MakeConstructorFunctionTemplate(
      sst.str().c_str())).ToLocalChecked(),
    args_info);
}

static void init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target) {
  Nan::Set(target, Nan::New<v8::String>("newHTTPServer").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(NewHTTPServer)->GetFunction());
}

NODE_MODULE(uvhttp, init)
