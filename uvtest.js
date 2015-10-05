var i = require('./');

setTimeout(function() {
i.newEventServer().newHTTPServer().uvtest();
}, 10);
