#include <nan_object_wrap_template.h>
#include <evhtp.h>
#include <iostream>

// made with *some* help from:
// - https://nodejs.org/api/addons.html
// - https://github.com/ellzey/libevhtp/blob/develop/README.markdown
// - https://github.com/brodybits/nan/blob/cb-object-wrapper-template/test/cpp/wrappedobjectfactory.cpp
//   which is based on:
//   https://github.com/nodejs/nan/blob/master/test/cpp/objectwraphandle.cpp

void mycb(evhtp_request_t * r, void *) {
  evbuffer_add_reference(r->buffer_out, "asdf", 4, NULL, NULL);
  evhtp_send_reply(r, EVHTP_RES_OK);
}

Nan::NAN_METHOD_RETURN_TYPE // void
mytest(Nan::NAN_METHOD_ARGS_TYPE args_info) {
  std::cout << "got mytest()" << std::endl;

  int port = 8000;

  // following libhtp sample:
  evbase_t * b = event_base_new();
  evhtp_t * h = evhtp_new(b, NULL);
  evhtp_set_cb(h, "/", mycb, NULL);
  evhtp_bind_socket(h, "0.0.0.0", port, 1024);
  event_base_loop(b, 0);
}

class MyEventServer : public ObjectWrapTemplate<MyEventServer> {
public:
  MyEventServer(Nan::NAN_METHOD_ARGS_TYPE) {
    testval=123;
    // XXX TODO CLEANUP
    evbase = event_base_new();
  }
  ~MyEventServer() {}

  static void Init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE ignored) {
    function_template tpl =
      NewConstructorFunctionTemplate("MyEventServer", 1);
    SetPrototypeMethod(tpl, "testMethod", TestMethod);
    SetPrototypeMethod(tpl, "newHTTPServer", NewHTTPServer);
    SetPrototypeMethod(tpl, "loop", Loop);
    SetConstructorFunctionTemplate(tpl);
  }

  static void NewInstance(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    NewInstanceMethod(args_info);
  }

private:
  static void TestMethod(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    std::cout << "got test method call" << std::endl;
  }

  static void NewHTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info);

  static void Loop(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    std::cout << "got loop method call" << std::endl;
    MyEventServer * myself = ObjectFromMethodArgsInfo(args_info);
    event_base_loop(myself->evbase, 0);
  }

public: // XXX TODO find a better way to share:
  evbase_t * evbase;
  int testval;
};

class HTTPServer : public ObjectWrapTemplate<HTTPServer> {
public:
  static void Init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE ignored) {
    function_template tpl =
      NewConstructorFunctionTemplate("HTTPServer", 1);
    SetConstructorFunctionTemplate(tpl);
  }

  HTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    if (args_info.Length() >= 1) {
      std::cout << "new http server" << std::endl;
      //v8::Local<MyEventServer> l = Nan::New<MyEventServer>(args_info[0]->ToObject());
      //MyEventServer * evs = node::ObjectWrap::Unwrap<MyEventServer>(
      //  Nan::New<MyEventServer>(args_info[0]->ToObject()));
      MyEventServer * evs = Unwrap<MyEventServer>(args_info[0]->ToObject());
      std::cout << "check testval: " << evs->testval << std::endl;
      evh = evhtp_new(evs->evbase, NULL);

      // XXX TODO MOVE:
      // following libhtp sample:
      evhtp_set_cb(evh, "/", mycb, NULL);
      evhtp_bind_socket(evh, "0.0.0.0", 8080, 1024);
      std::cout << "new http done" << std::endl;
    }
  }

  ~HTTPServer() {}

  static v8::Local<v8::Value> NewInstance(int argc, v8::Local<v8::Value> argv[]) {
    return NewInstanceMethod(argc, argv);
  }

  evhtp_t * evh;
};

void MyEventServer::NewHTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
  //v8::Local<v8::Value> argv[1] = { args_info.This() };
  v8::Local<v8::Value> lv = args_info.This();
  v8::Local<v8::Value> argv[1] = { lv };
  //args_info.GetReturnValue().Set(HTTPServer::NewInstance(1, argv));
  //args_info.GetReturnValue().Set(HTTPServer::NewInstance(0, NULL));
  args_info.GetReturnValue().Set(HTTPServer::NewInstance(1, argv));
}

void init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target) {
  Nan::Set(target, Nan::New<v8::String>("mytest").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(mytest)->GetFunction());
  MyEventServer::Init(target);
  HTTPServer::Init(target);
  Nan::Set(target, Nan::New<v8::String>("newEventServer").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(MyEventServer::NewInstance)->GetFunction());
}

NODE_MODULE(evhtp, init)
