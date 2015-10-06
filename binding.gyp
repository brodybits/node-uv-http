{
  "targets": [
    {
      "target_name": "node-uv-http",
      "sources": [
        "evhtp-all.c",
        "binding.cc"
      ],
      "include_dirs": [
        "<!(node -e \"require ('nan')\")",
        ".",
        "libevhtp",
        "libevent-2.0.22-stable",
        "libevent-2.0.22-stable/include"
      ],
      "defines": [
        "EVHTP_SYS_ARCH=64",
        "EVHTP_DISABLE_REGEX",
        "EVHTP_DISABLE_SSL"
      ],
      "libraries": [
        "../libevent-2.0.22-stable/.libs/libevent_core.a"
      ]
    }
  ]
}