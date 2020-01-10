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

const client = new something_proto.Something('localhost:7777',
                                             grpc.credentials.createInsecure());

client.doit({input: 'bajja'}, function(err, response) {
  console.log(`Output from server:[${response.output}]`);
});
