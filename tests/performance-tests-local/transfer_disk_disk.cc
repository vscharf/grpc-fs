#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <chrono>
#include <fstream>
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
  FileServerImpl(std::size_t chunk_size)
    : FileServer::Service()
    , chunk_size_(chunk_size)
    , ptr_(new char[chunk_size])
  {
    std::cout << "chunk_size = " << chunk_size_ << std::endl;
  }

  ~FileServerImpl() { delete[] ptr_; }

  Status GetFile(ServerContext* context, const FileRequest* request,
                 ServerWriter<FileChunk>* writer) override
  {
    FileChunk fc;
    
    std::ifstream F(request->name());
    while(F.read(ptr_, chunk_size_)) {
      fc.set_chunk(ptr_, chunk_size_);
      writer->Write(fc);
    }
    fc.set_chunk(ptr_, F.gcount());
    writer->Write(fc);

    return Status::OK;
  }
private:
  const std::size_t chunk_size_;
  char *ptr_;
};

void RunServer(std::size_t chunk_size) {
  std::string server_address("0.0.0.0:50051");
  FileServerImpl service(chunk_size);

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

  bool GetFile(std::string remoteFile, std::string localFile) {
    ClientContext context;

    FileChunk c;
    FileRequest r;
    r.set_name(remoteFile);

    std::unique_ptr<ClientReader<FileChunk> > reader(stub_->GetFile(&context, r));
    std::ofstream F(localFile);
    while (reader->Read(&c)) {
      payload += c.chunk().size();
      F.write(c.chunk().data(), c.chunk().size());
    }
    F.close();

    Status status = reader->Finish();
    return status.IsOk();
  }
  void Shutdown() { stub_.reset(); }

  std::size_t payload = 0u;

private:
  std::unique_ptr<FileServer::Stub> stub_;
};

void ClientGetFile(std::string remote, std::string local, std::string address = "0.0.0.0:50051") {
  grpc_init();
  
  FileServerClient client(grpc::CreateChannel(address, grpc::InsecureCredentials(),
                                              ChannelArguments()));
  
  std::cout << "-------------- GetFile --------------" << std::endl;
  auto start = steady_clock::now();
  bool ok = client.GetFile(remote, local);
  auto end = steady_clock::now();
  
  std::cout << "Result: " << ok << " time: " << duration<double, std::milli>(end - start).count() << " ms" << std::endl;
  std::cout << "Rate:   " << client.payload/1024./1024./duration<double>(end - start).count() << " MB/s" << std::endl;
  
  client.Shutdown();
}

int main(int argc, char** argv) {
  if(argc < 3 || argc > 4) {
    std::cerr << "Two positional arguments expected: <remote-file> <local-file> [<chunk-size>=4096]" << std::endl;
    return 1;
  }

  std::size_t chunk_size = 4096;
  if(argc == 4) {
    chunk_size = std::stoull(argv[3]);
  }

  pid_t server_pid = fork();
  if(server_pid == 0) {
    // server thread (child)
    
    grpc_init();
    
    RunServer(chunk_size);
    
    grpc_shutdown();
    return 0;
  } else {
    // client thread (parent)

    // wait some time to allow server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ClientGetFile(argv[1], argv[2]);

    if(server_pid) {
      // wait to give the other client time to retrieve the file in case we are early
      kill(server_pid, SIGTERM);
    }
  }

  grpc_shutdown();
}
