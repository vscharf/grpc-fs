#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <thread>
#include <mutex>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "src/cpp/server/thread_pool.h"
#include "grpc_fs.pb.h"

#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "util/pipeline.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpcfs::FileRequest;
using grpcfs::FileRequestAnswer;
using grpcfs::FileServer;

class FileServerImpl final : public FileServer::Service {
public:
  FileServerImpl(std::size_t n_fetch, std::size_t n_transfer) : pipeline_(n_fetch, n_transfer) {}
  Status GetFile(ServerContext* context, const ::grpcfs::FileRequest* request, ::grpcfs::FileRequestAnswer* response) override
  {
    std::string n = request->name();
    std::string h = request->host();
    int p = request->port();

    pipeline_.push(n, h, p);

    response->set_status(grpcfs::FileRequestAnswer_Status_OK);

    return Status::OK;
  }
private:
  grpcfs::util::Pipeline pipeline_;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  FileServerImpl service(4, 8);

  grpc::ThreadPool thread_pool(2);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.SetThreadPool(&thread_pool);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cerr << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  grpc_init();
    
  RunServer();
    
  grpc_shutdown();
    
  return 0;
}
