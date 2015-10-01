var evhtp = require('./');

var evserver = evhtp.newEventServer();
evserver.testMethod();

//evhtp.mytest();

var hs = evserver.newHTTPServer();
evserver.loop();
