#include <iostream>
#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "grpc_fs.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpcfs::FileRequest;
using grpcfs::FileChunk;
using grpcfs::FileServer;

class FileServerImpl final : public FileServer::Service {
public:
  Status GetFile(ServerContext* context, const FileRequest* request,
                 ServerWriter<FileChunk>* writer) override
  {
    std::cout << "Client requested file: " << request->name() << std::endl;
    FileChunk fc;
    fc.set_chunk("some chunk");
    writer->Write(fc);
    return Status::OK;
  }
private:
  
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FileServerImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  grpc_init();

  RunServer();

  grpc_shutdown();
  return 0;
}
