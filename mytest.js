var uvhttp = require('./');
var p = require('process'); // Node.js/io.js 1.0 or greater
var http = require('http');
var tap = require('tap');

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

tap.test('static path test', function(t) {
  // thanks: http://davidwalsh.name/nodejs-http-request
  var contents='';

  http.get({
    host: 'localhost',
    path: '/static',
    port: 8080
  }, function(resp) {
    resp.on('data', function(data) {
      console.log('got data: ' + data);
      contents += data;
    });
    resp.on('end', function() {
      console.log('got end, total contents: ' + contents);
      t.equal(contents, "Static content from /static path\n", '/static contents');
      t.end();
    })
  });
});

tap.test('root path test', function(t) {
  // thanks: http://davidwalsh.name/nodejs-http-request
  var contents='';

  http.get({
    host: 'localhost',
    path: '/',
    port: 8080
  }, function(resp) {
    resp.on('data', function(data) {
      console.log('got data: ' + data);
      contents += data;
    });
    resp.on('end', function() {
      console.log('got end, total contents: ' + contents);
      t.equal(contents, "Static content from root path\n", 'root contents');
      t.end();
    })
  });
});

tap.test('Immediate Javascript callback test', function(t) {
  var contents='';

  http.get({
    host: 'localhost',
    path: '/test',
    port: 8080
  }, function(resp) {
    resp.on('data', function(data) {
      console.log('got data: ' + data);
      contents += data;
    });
    resp.on('end', function() {
      console.log('got end, total contents: ' + contents);
      t.equal(contents, 'Response from /test Javascript callback\n', '/test2 contents');
      t.end();
    })
  });
});

tap.test('Javascript callback in the next tick', function(t) {
  var contents='';

  http.get({
    host: 'localhost',
    path: '/test2',
    port: 8080
  }, function(resp) {
    resp.on('data', function(data) {
      console.log('got data: ' + data);
      contents += data;
    });
    resp.on('end', function() {
      console.log('got end, total contents: ' + contents);
      t.equal(contents, 'Response from /test2 Javascript callback\n', '/test2 contents');
      t.end();
    })
  });
});

tap.test('stop', function(t) {
  http.get({
    host: 'localhost',
    path: '/stop',
    port: 8080
  });
  t.end();
});
