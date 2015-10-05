var i = require('./');

//setTimeout(function() {
//i.newEventServer().newHTTPServer().uvtest();
//}, 10);

//i.newHTTPServer().uvtest();
var myhost = "0.0.0.0",
    myport = 8080;

//i.newHTTPServer().bindAddr(myhost, myport, 111);

var httpServer = i.newHTTPServer();
httpServer.bindAddr(myhost, myport, 111);
httpServer.staticPath("/", 200, "Content from root path\n");
httpServer.pathCB("/test", function(r) {
  console.log('asdf');
  r.res(200, 'Response from /test Javascript callback\n');
});

httpServer.pathCB("/test2", function(r) {
  console.log('asdf');
  setTimeout(function() {
    r.res(200, 'Response from /test2 Javascript callback\n');
  }, 0)
});
