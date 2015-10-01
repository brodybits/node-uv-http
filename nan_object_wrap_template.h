 /*********************************************************************
  * Derived from nan_object_wrap.h from
  * NAN - Native Abstractions for Node.js
  *
  * Copyright (c) 2015 Christopher J. Brody
  * Copyright (c) 2015 NAN contributors
  *
  * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
  ********************************************************************/

#include <nan.h>

#ifndef NAN_OBJECT_WRAP_TEMPLATE_H_
#define NAN_OBJECT_WRAP_TEMPLATE_H_

template <typename T>
class ObjectWrapTemplate : public Nan::ObjectWrap {
 protected:
  typedef v8::Local<v8::FunctionTemplate> function_template;

  static inline function_template
  NewConstructorFunctionTemplate(const char * object_name, int internal_field_count) {
    function_template tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New(object_name).ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);
    return tpl;
  }

  static inline void
  SetConstructorFunctionTemplate(function_template tpl) {
    constructor().Reset(tpl->GetFunction());
  }

  static inline void
  SetNewFunction(Nan::ADDON_REGISTER_FUNCTION_ARGS_TYPE target,
                 function_template tpl, const char * object_name) {
    Nan::Set(target, Nan::New(object_name).ToLocalChecked(), tpl->GetFunction());
  }

  ObjectWrapTemplate() {}
  ~ObjectWrapTemplate() {}

  // Used by NewConstructorFunctionTemplate():
  // (same as static NAN_METHOD(New) { ... })
  static void New(Nan::NAN_METHOD_ARGS_TYPE info) {
    if (info.IsConstructCall()) {
      T *obj = new T(info);
      obj->Wrap(info.This());
      info.GetReturnValue().Set(info.This());
    } else {
      NewInstanceMethod(info);
    }
  }

  // ref: https://nodejs.org/api/addons.html#addons_factory_of_wrapped_objects
  // (same as static NAN_METHOD(NewInstanceMethod) { ... })
  static inline void NewInstanceMethod(Nan::NAN_METHOD_ARGS_TYPE info) {
    v8::Local<v8::Function> cons = Nan::New(constructor());
    const int argc = info.Length();
    std::vector<v8::Local<v8::Value> > argv;
    for (int i=0; i<argc; ++i) argv.push_back(info[i]);
    info.GetReturnValue().Set(cons->NewInstance(argc, &argv[0]));
  }
  static inline v8::Local<v8::Value> NewInstanceMethod(int argc, v8::Local<v8::Value> argv[]) {
    v8::Local<v8::Function> cons = Nan::New(constructor());
    return cons->NewInstance(argc, argv);
  }

  static inline T * ObjectFromMethodArgsInfo(Nan::NAN_METHOD_ARGS_TYPE info) {
    return ObjectWrap::Unwrap<T>(info.This());
  }

 private:
  static inline Nan::Persistent<v8::Function> & constructor() {
    static Nan::Persistent<v8::Function> constructor_;
    return constructor_;
  }
};

#endif  // NAN_OBJECT_WRAP_TEMPLATE_H_
