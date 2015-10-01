var evhtp = require('./');

var myhost = "0.0.0.0",
    myport = 8080,
    mybacklog = 1024;

var evserver = evhtp.newEventServer();
var httpServer = evserver.newHTTPServer();
httpServer.bindSocket(myhost, myport, mybacklog);

console.log("HTTP server listening to port: " + myport)
evserver.loop();
