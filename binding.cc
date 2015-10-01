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

class HTTPServer : public ObjectWrapTemplate<HTTPServer> {
public:
  static void Init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE ignored) {
    function_template tpl =
      NewConstructorFunctionTemplate("HTTPServer", 1);
    SetPrototypeMethod(tpl, "bindSocket", BindSocket);
    SetConstructorFunctionTemplate(tpl);
  }

  HTTPServer(Nan::NAN_METHOD_ARGS_TYPE args_info) {
    if (args_info.Length() >= 1) {
      MyEventServer * evs = Unwrap<MyEventServer>(args_info[0]->ToObject());
      evh = evhtp_new(evs->evbase, NULL);

      // XXX TODO MOVE:
      evhtp_set_cb(evh, "/", mycb, NULL);
    }
  }

  ~HTTPServer() {}

  static v8::Local<v8::Value> NewInstance(int argc, v8::Local<v8::Value> argv[]) {
    return NewInstanceMethod(argc, argv);
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
  HTTPServer::Init(target);
  Nan::Set(target, Nan::New<v8::String>("newEventServer").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(MyEventServer::NewInstance)->GetFunction());
}

NODE_MODULE(evhtp, init)
