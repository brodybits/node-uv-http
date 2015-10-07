# node-uvhttp

Node HTTP server library that serves static content from native code.

Author: Christopher J. Brody

License: MIT

I can hereby testify that this project is completely my own work and not subject to agreements with any other parties.
The exception is `nan_object_wrap_template.h` which is derived from the Node.js Nan project.
In case of code written under direct guidance from sample code the link is given for reference.
In case I accept contributions from any others I will require CLA with similar statements.
I may offer this work under other licensing terms in the future.

WARNING: This project is under development and should be considered experimental.
API is subject to change and some optimizations may be needed.

Major TODOs:
- Automatic testing
- BROKEN: Incoming HTTP request framing
- BROKEN: status codes
- Error checking
- Content-Type header
- Support binary data using Buffer
- Support Keep-Alive

Highly desired:
- Multi-threading support, using node-webworker-threads and/or JXCore

Other features under consideration:
- HTTPS
- HTTP/2

## Usage

var uvhttp = require('./node-uvhttp');

var myhost = "0.0.0.0",
    myport = 8080;

var httpServer = uvhttp.newHTTPServer();
httpServer.bindAddr(myhost, myport, 111);

httpServer.staticPath("/static", 200, "Static content from /static path\n");

httpServer.pathCB("/test", function(r) {
  r.res(200, 'Response from /test Javascript callback\n');
});

httpServer.pathCB("/test2", function(r) {
  setTimeout(function() {
    r.res(200, 'Response from /test2 Javascript callback\n');
  }, 0)
});

httpServer.staticPath("/", 200, "Static content from root path\n");

## Local test

```shell
npm install
```

To rebuild:

```shell
node-gyp rebuild
```

To run:

```shell
node mytest.js
```

In another window:

Static content from native code:

```shell
curl http://localhost:8080
```

Content from Javascript callback:

```shell
curl http://localhost:8080/test
```

Content from Javascript callback in the next tick:

```shell
curl http://localhost:8080/test2
```

To stop the test program:
```shell
curl http://localhost:8080/stop
```
