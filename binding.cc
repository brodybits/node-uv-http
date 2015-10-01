#include <nan.h>
#include <evhtp.h>
#include <iostream>

// made with *some* help from:
// - https://nodejs.org/api/addons.html
// - https://github.com/ellzey/libevhtp/blob/develop/README.markdown

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

void init(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target) {
  Nan::Set(target, Nan::New<v8::String>("mytest").ToLocalChecked(),
           Nan::New<v8::FunctionTemplate>(mytest)->GetFunction());
}

NODE_MODULE(evhtp, init)
