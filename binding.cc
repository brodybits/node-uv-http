#include <node.h>
#include <evhtp.h>
#include <iostream>

// made with *some* help from:
// - https://nodejs.org/api/addons.html
// - https://github.com/ellzey/libevhtp/blob/develop/README.markdown

void mycb(evhtp_request_t * r, void *) {
  evbuffer_add_reference(r->buffer_out, "asdf", 4, NULL, NULL);
  evhtp_send_reply(r, EVHTP_RES_OK);
}

void mytest(const v8::FunctionCallbackInfo<v8::Value> & cbinfo) {
  std::cout << "got mytest()" << std::endl;

  int port = 8000;

  // following libhtp sample:
  evbase_t * b = event_base_new();
  evhtp_t * h = evhtp_new(b, NULL);
  evhtp_set_cb(h, "/", mycb, NULL);
  evhtp_bind_socket(h, "0.0.0.0", port, 1024);
  event_base_loop(b, 0);
}

void init(v8::Local<v8::Object> exports) {
  NODE_SET_METHOD(exports, "mytest", mytest);
}

NODE_MODULE(evhtp, init)
