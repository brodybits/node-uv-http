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

Also works on [JXCore](http://jxcore.io/) with multi-threading.

Major TODOs:
- IPv6
- (Travis) CI
- Real benchmarking
- BROKEN: Incoming HTTP request framing
- BROKEN: only supports HTTP GET
- BROKEN: status codes
- Error checking
- Content-Type
- Incoming HTTP header parsing
- Support outgoing HTTP response headers
- Support binary data using Buffer
- Support Keep-Alive
- Support an interface similar to Node.js built-in HTTP server, perhaps in a higher-level library
- CLEANUP remove developer test logging statements
- send static content from file instead of memory using sendfile

Highly desired:
- Web socket
- Integration with a higher-level framework such as express.js

Other features under consideration:
- HTTPS
- HTTP/2

## Usage

```Javascript
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
```

## Local test

```shell
npm install
```

To rebuild:

```shell
node-gyp rebuild
```

To run test:

```shell
npm test
```

## Preliminary CPU time comparison

### Test system

These tests are performed on my MacBook Air.

### Test program

```Javascript
var uvhttp = require('./');
var http = require('http');

// based on: http://blog.modulus.io/build-your-first-http-server-in-nodejs
const PORT=8080;

function handleRequest(request, response){
    response.end('response\n');
}

var server = http.createServer(handleRequest);
server.listen(PORT, function(){
    console.log("Server listening on: http://localhost:%s", PORT);
});

// node-uvhttp server:
var myhost = "0.0.0.0",
    myport = 9090;

var httpServer = uvhttp.newHTTPServer();
httpServer.bindAddr(myhost, myport, 111);

httpServer.staticPath("/static", 200, "Static content from /static path\n");

httpServer.pathCB("/test", function(r) {
  r.res(200, 'Response from /test Javascript callback\n');
});

console.log("HTTP server listening to port: " + myport)
```

### Results for built-in Node HTTP server

Test command (Apache ab sending 10K HTTP requests):

```shell
$ ab -n 10000 http://0.0.0.0:8080/
```

CPU time measurement:

```shell
$ time node benchmark-server.js
HTTP server listening to port: 9090
Server listening on: http://localhost:8080
^C

real	0m7.759s
user	0m1.734s
sys	0m0.549s
```

### Results for static path query in this project

Test command (10K HTTP requests):

```shell
$ ab -n 10000 http://0.0.0.0:9090/static
```

CPU time measurement:

```shell
$ time node benchmark-server.js
HTTP server listening to port: 9090
Server listening on: http://localhost:8080
^C

real	0m5.704s
user	0m0.355s
sys	0m0.426s
```

### Results for Javascript path query callback in this project

Test command (10K HTTP requests):

```shell
$ ab -n 10000 http://0.0.0.0:9090/test
```

CPU time measurement:

```shell
$ time node benchmark-server.js
HTTP server listening to port: 9090
Server listening on: http://localhost:8080
^C

real	0m5.507s
user	0m0.510s
sys	0m0.473s
```

### Another test: response in the next tick

```Javascript
var uvhttp = require('./');
var http = require('http');

// based on: http://blog.modulus.io/build-your-first-http-server-in-nodejs
const PORT=8080;

function handleRequest(request, response){
  setTimeout(function() {
    response.end('response\n');
  }, 0)
}

var server = http.createServer(handleRequest);
server.listen(PORT, function(){
    console.log("Server listening on: http://localhost:%s", PORT);
});

// node-uvhttp server:
var myhost = "0.0.0.0",
    myport = 9090;

var httpServer = uvhttp.newHTTPServer();
httpServer.bindAddr(myhost, myport, 111);

httpServer.pathCB("/test2", function(r) {
  setTimeout(function() {
    r.res(200, 'Response from /test2 Javascript callback\n');
  }, 0)
});

console.log("HTTP server listening to port: " + myport)
```

#### Results for built-in Node HTTP server

Test command (10K HTTP requests):

```shell
$ ab -n 10000 http://0.0.0.0:8080/
```

CPU time measurement:

```shell
$ time node benchmark-server.js
HTTP server listening to port: 9090
Server listening on: http://localhost:8080
^C

real	0m27.319s
user	0m4.933s
sys	0m1.468s
```

#### Results for this project

Test command (10K HTTP requests):

```shell
$ ab -n 10000 http://0.0.0.0:9090/test2
```

CPU time measurement:

```shell
$ time node benchmark-server.js
HTTP server listening to port: 9090
Server listening on: http://localhost:8080
^C

real	0m22.301s
user	0m2.015s
sys	0m1.299s
```

### Comparison and analysis

This project uses significantly less CPU time than the standard Node.js
HTTP module, at least for simple HTTP GET queries with string results
(other functionalty to be added in the near future).
This is especially true for static content served within the native module,
and is also true when the Javascript is called to send dynamic content.

While CPU time may not be a serious issue for I/O-bound systems, CPU time can
also become extremely important for common Node.js applications such as web
servers. In general, it is recommended to place a front-end proxy such as
`nginx` in front of a Node.js application to serve static content and deal with
other features such as compression, HTTPS, and HTTP/2 (or SPDY).
A goal of this project is to alleviate the need for such front-end proxies.

Especially if this project is use to serve both static and dynamic content,
multi-processing will be very important, using either multiple processes or
multiple threads. Having multiple processes listening to the same port
has been reported to cause problems under certain circumstances.
Multi-threading is the recommended approach, which is now working with [JXCore](http://jxcore.io/).
