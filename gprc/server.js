const grpc = require('grpc');
const protoLoader = require('@grpc/proto-loader');

const packageDefinition = protoLoader.loadSync(
    `${__dirname}/something.proto`,
    {keepCase: true,
     longs: String,
     enums: String,
     defaults: true,
     oneofs: true
    });

const something_proto = grpc.loadPackageDefinition(packageDefinition).lkn;

function doit(call, callback) {
  console.log(`doit input:[${call.request.input}]`);
  callback(null, {output: `Processed input ${call.request.input}`});
}

const server = new grpc.Server();
server.addService(something_proto.Something.service, {doit: doit});
server.bind('0.0.0.0:7777', grpc.ServerCredentials.createInsecure());
server.start();
