#include <nan_object_wrap_template.h>
#include <evhtp.h>
#include <iostream>

// made with *some* help from:
// - https://nodejs.org/api/addons.html
// - https://github.com/ellzey/libevhtp/blob/develop/README.markdown
// - https://github.com/brodybits/nan/blob/cb-object-wrapper-template/test/cpp/wrappedobjectfactory.cpp
//   which is based on:
//   https://github.com/nodejs/nan/blob/master/test/cpp/objectwraphandle.cpp

class MyEventServer : public ObjectWrapTemplate<MyEventServer> {
public:
  MyEventServer(Nan::NAN_METHOD_ARGS_TYPE) {
    // XXX TODO CLEANUP evbase object
    evbase = event_base_new();
  }
  ~MyEventServer() {}

  static void Init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE ignored) {
    function_template tpl =
      NewConstructorFunctionTemplate("MyEventServer", 1);
    SetPrototypeMethod(tpl, "newHTTPServer", NewHTTPServer);
    SetPrototypeMethod(tpl, "loop", Loop);
    SetConstructorFunctionTemplate(tpl);
  }

  static void NewInstance(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    NewInstanceMethod(args_info);
  }

private:
  static void NewHTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info);

  static void Loop(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    MyEventServer * myself = ObjectFromMethodArgsInfo(args_info);
    event_base_loop(myself->evbase, 0);
  }

public: // XXX TODO find a better way to share:
  evbase_t * evbase;
};

struct StaticPathInfo {
  StaticPathInfo(const char * path, int code, const char * content) :
    path(path), code(code), content(content) {}

  std::string path;
  int code;
  std::string content;
};

struct PathCBInfo {
  // XXX TODO: should store & use V8 isolate value:
  PathCBInfo(v8::Local<v8::Function> & f) : pf(f) {}

  Nan::Persistent<v8::Function> pf;
};

class HTTPServerReq : public ObjectWrapTemplate<HTTPServerReq> {
public:
  static void Init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE ignored) {
    function_template tpl =
      NewConstructorFunctionTemplate("HTTPServerReq", 1);
    SetPrototypeMethod(tpl, "res", res);
    SetConstructorFunctionTemplate(tpl);
  }

  HTTPServerReq(Nan::NAN_METHOD_ARGS_TYPE args_info) : info(NULL), r(NULL) {}

  ~HTTPServerReq() {}

  static v8::Local<v8::Value> NewInstance(int argc, v8::Local<v8::Value> argv[]) {
    return NewInstanceMethod(argc, argv);
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
    evbuffer_add_reference(myself->r->buffer_out, cs.c_str(), cs.length(), NULL, NULL);
    evhtp_send_reply(myself->r, args_info[0]->Int32Value());
  }

  PathCBInfo * info;
  evhtp_request_t * r;
};

class HTTPServer : public ObjectWrapTemplate<HTTPServer> {
public:
  static void Init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE ignored) {
    function_template tpl =
      NewConstructorFunctionTemplate("HTTPServer", 1);
    SetPrototypeMethod(tpl, "staticPath", StaticPath);
    SetPrototypeMethod(tpl, "pathCB", PathCB);
    SetPrototypeMethod(tpl, "bindSocket", BindSocket);
    SetConstructorFunctionTemplate(tpl);
  }

  HTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    if (args_info.Length() >= 1) {
      MyEventServer * evs = Unwrap<MyEventServer>(args_info[0]->ToObject());
      evh = evhtp_new(evs->evbase, NULL);
    }
  }

  ~HTTPServer() {}

  static v8::Local<v8::Value> NewInstance(int argc, v8::Local<v8::Value> argv[]) {
    return NewInstanceMethod(argc, argv);
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
    StaticPathInfo * info =
      new StaticPathInfo(mypath.c_str(), args_info[1]->Int32Value(), mycontent.c_str());

    evhtp_set_cb(myself->evh, mypath.c_str(), StaticCB, reinterpret_cast<void *>(info));
  }

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
    // - TBD ???: free the memory in case this static path is no longer relevant
    // - TBD ???: get rid of extra std::string storage to make this more efficient
    v8::Local<v8::Function> f = v8::Local<v8::Function>::Cast(args_info[1]);
    PathCBInfo * info = new PathCBInfo(f);

    evhtp_set_cb(myself->evh, mypath.c_str(), PathCBCall, reinterpret_cast<void *>(info));
  }

  static void StaticCB(evhtp_request_t * r, void * p) {
    StaticPathInfo * info = reinterpret_cast<StaticPathInfo *>(p);

    evbuffer_add_reference(r->buffer_out, info->content.c_str(), info->content.size(), NULL, NULL);
    evhtp_send_reply(r, info->code);
  }

  static void PathCBCall(evhtp_request_t * r, void * p) {
    PathCBInfo * info = reinterpret_cast<PathCBInfo *>(p);

    //evbuffer_add_reference(r->buffer_out, info->content.c_str(), info->content.size(), NULL, NULL);
    //evhtp_send_reply(r, info->code);

    v8::Local<v8::Value> sr_argv[1] = {Nan::New(1)};
    v8::Local<v8::Object> sr = HTTPServerReq::NewInstance(1, sr_argv)->ToObject();

    HTTPServerReq * mysr = ObjectWrap::Unwrap<HTTPServerReq>(sr);
    mysr->info = info;
    mysr->r = r;

    static int argc = 1;
    v8::Local<v8::Value> argv[1] = {sr};

    v8::Local<v8::Function> f = Nan::New(info->pf);
    f->Call(Nan::Null(), argc, argv);
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

    evhtp_bind_socket(myself->evh, *v8::String::Utf8Value(args_info[0]->ToString()),
                      args_info[1]->Int32Value(), args_info[2]->Int32Value());
  }

  evhtp_t * evh;
};

void MyEventServer::NewHTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
  v8::Local<v8::Value> lv = args_info.This();
  v8::Local<v8::Value> argv[1] = { lv };
  args_info.GetReturnValue().Set(HTTPServer::NewInstance(1, argv));
}

void init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target) {
  MyEventServer::Init(target);
  HTTPServerReq::Init(target);
  HTTPServer::Init(target);
  Nan::Set(target, Nan::New<v8::String>("newEventServer").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(MyEventServer::NewInstance)->GetFunction());
}

NODE_MODULE(evhtp, init)
