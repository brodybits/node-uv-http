var i = require('./');

var myhost = "0.0.0.0",
    myport = 8080;

var httpServer = i.newHTTPServer();
httpServer.bindAddr(myhost, myport, 111);

httpServer.pathCB("/test", function(r) {
  r.res(200, 'Response from /test Javascript callback\n');
});

httpServer.staticPath("/", 200, "Content from root path\n");

httpServer.pathCB("/test2", function(r) {
  setTimeout(function() {
    r.res(200, 'Response from /test2 Javascript callback\n');
  }, 0)
});

console.log("HTTP server listening to port: " + myport)
