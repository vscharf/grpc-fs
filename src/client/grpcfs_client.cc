#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "grpc_fs.pb.h"

using grpc::ChannelArguments;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using grpcfs::FileServer;
using grpcfs::FileChunk;
using grpcfs::FileRequest;

class FileServerClient {
public:
  FileServerClient(std::shared_ptr<ChannelInterface> channel)
    : stub_(FileServer::NewStub(channel)) {}

  void GetFile() {
    ClientContext context;

    FileChunk c;
    FileRequest r;
    r.set_name("/some/file");
    std::cout << "Requesting file " << r.name() << "\n";

    std::unique_ptr<ClientReader<FileChunk> > reader(stub_->GetFile(&context, r));
    while (reader->Read(&c)) {
      std::cout << "Read chunk of length: " << c.chunk().size()
                << "  -- ByteSize: " << c.ByteSize() << "\n";
    }

    Status status = reader->Finish();
    if (status.IsOk()) {
      std::cout << "GetFile rpc succeeded.\n";
    } else {
      std::cout << "GetFile rpc failed.\n";
    }
  }
  void Shutdown() { stub_.reset(); }

private:
  std::unique_ptr<FileServer::Stub> stub_;
};

int main(int argc, char** argv) {
  grpc_init();

  FileServerClient client(
      grpc::CreateChannel("localhost:50051", grpc::InsecureCredentials(),
                          ChannelArguments())
      );

  std::cout << "-------------- GetFile --------------\n";
  client.GetFile();

  client.Shutdown();

  grpc_shutdown();
}
