var uvhttp = require('./');
var p = require('process'); // Node.js/io.js 1.0 or greater

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

httpServer.pathCB("/stop", function(r) {
  r.res(200, '/stop callback, stopping server\n');
  console.log('/stop callback, stopping server');
  p.exit();
});

httpServer.staticPath("/", 200, "Static content from root path\n");

console.log("HTTP server listening to port: " + myport)
