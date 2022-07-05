const http = require('http');
const os = require('os');

console.log("Node.js example server starting...");

const handler = function(request, response) {
  console.log(`Received request from ${request.connection.remoteAddress}`);
  response.writeHead(200);
  response.end(`Container id (os.hostname): ${os.hostname()}\n`);
};

const server = http.createServer(handler);
server.listen(8080);
