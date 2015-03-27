#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpc++/channel_arguments.h>
#include <grpc++/channel_interface.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "grpc_fs.pb.h"

using grpc::ChannelArguments;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpcfs::FileServer;
using grpcfs::FileChunk;
using grpcfs::FileRequest;

using namespace std::chrono;

class FileServerImpl final : public FileServer::Service {
public:
  FileServerImpl(std::size_t chunk_size,
                 std::size_t total_size)
    : FileServer::Service()
    , chunk_size_(chunk_size)
    , total_size_(total_size)
    , ptr_(new char[chunk_size])
  {
    std::cout << "chunk_size = " << chunk_size_ << " -- total_size = " << total_size_ << std::endl;
  }

  ~FileServerImpl() { delete[] ptr_; }

  Status GetFile(ServerContext* context, const FileRequest* request,
                 ServerWriter<FileChunk>* writer) override
  {
    FileChunk fc;
    size_t transferred_size = 0u;
    while(transferred_size < (total_size_ - chunk_size_)) {
      fc.set_chunk(ptr_, chunk_size_);
      transferred_size += chunk_size_;
      writer->Write(fc);
    }
    return Status::OK;
  }
private:
  std::size_t chunk_size_;
  std::size_t total_size_;
  char *ptr_;
};

void RunServer(std::size_t chunk_size, std::size_t total_size) {
  std::string server_address("0.0.0.0:50051");
  FileServerImpl service(chunk_size, total_size);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

class FileServerClient {
public:
  FileServerClient(std::shared_ptr<ChannelInterface> channel)
    : stub_(FileServer::NewStub(channel)) {}

  bool GetFile() {
    ClientContext context;

    FileChunk c;
    FileRequest r;
    r.set_name("/some/file");

    std::unique_ptr<ClientReader<FileChunk> > reader(stub_->GetFile(&context, r));
    while (reader->Read(&c)) {
      payload += c.chunk().size();
      total += c.ByteSize();
    }

    Status status = reader->Finish();
    return status.IsOk();
  }
  void Shutdown() { stub_.reset(); }

  std::size_t payload = 0;
  std::size_t total = 0;

private:
  std::unique_ptr<FileServer::Stub> stub_;
};

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Two positional arguments expected: <chunk_size> <total_size>" << std::endl;
    return 1;
  }

  auto chunk_size = std::stoull(argv[1]);
  auto total_size = std::stoull(argv[2]);

  pid_t pid = fork();
  if(pid == 0) {
    // server thread (child)
    
    grpc_init();
    
    RunServer(chunk_size, total_size);
    
    grpc_shutdown();
    return 0;
  } else {
    // wait some time to allow server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // client thread (parent)
    grpc_init();

    FileServerClient client(
                            grpc::CreateChannel("localhost:50051", grpc::InsecureCredentials(),
                                                ChannelArguments())
                            );
    
    std::cout << "-------------- GetFile --------------\n";
    auto start = steady_clock::now();
    bool ok = client.GetFile();
    auto end = steady_clock::now();
    
    std::cout << "Result: " << ok << '\n';
    std::cout << duration<double, std::milli>(end - start).count() << " ms\n";
    std::cout << "Payload: " << client.payload/1024./1024. << " MB\n";
    std::cout << "Total:   " << client.total/1024./1024. << " MB\n";
    std::cout << "Eff:     " << double(client.payload)/client.total << '\n';
    
    client.Shutdown();

    kill(pid, SIGTERM);
  }

  grpc_shutdown();
}
