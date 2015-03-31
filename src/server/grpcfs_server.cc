#include <fstream>
#include <iostream>
#include <memory>
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
#include "grpc_fs.pb.h"

#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpcfs::FileRequest;
using grpcfs::FileRequestAnswer;
using grpcfs::FileServer;

template<std::streamsize BLOCK_SIZE>
class FileServerImpl final : public FileServer::Service {
public:
  Status GetFile(ServerContext* context, const ::grpcfs::FileRequest* request, ::grpcfs::FileRequestAnswer* response) override
  {
    std::string n = request->name();
    std::string h = request->host();
    int p = request->port();

    {
      std::lock_guard<std::mutex> lg(cout_mutex_);
      std::cerr << "Request for file " << n << " from " << h << " to port " << p << std::endl;
      (std::cout << n << " " << h << " " << p << " ").flush();
      std::cerr << "... submitted" << std::endl;
    }

    response->set_status(grpcfs::FileRequestAnswer_Status_OK);

    return Status::OK;
  }
private:
  char P_[BLOCK_SIZE];
  std::mutex cout_mutex_;
};

class Spawner
{
public:
  static void Start() {
    while(1) {
      std::string n, h;
      int p;
      std::cin >> n >> h >> p;
      if(!std::cin) {
        std::cerr << "Error reading from STDIN!\n";
        std::cin >> std::cerr.rdbuf();
        std::cerr << std::endl;
        return;
      }

      std::cerr << "Spawning thread for " << h << ":" << p << " ..." << n << std::endl; 
      std::thread t(spawn<4096>, n, h, p);
      t.join();
      std::cerr << "... done." << std::endl; 
    }
  }
private:
  template <std::streamsize BLOCK_SIZE>
  static void spawn(std::string n, std::string h, int p) {
    char P_[BLOCK_SIZE];
    FILE* pipe_fp;
    if((pipe_fp = popen(("nc -v -q 0 " + h + " " + std::to_string(p) + " 1>nc.log 2>nc.err").c_str(), "we")) == NULL) {
      perror("popen");
      std::cerr << "popen failed" << std::endl;
      return;
    }
    
    FILE* F = fopen(n.c_str(), "re");
    std::size_t N(0);
    while((N = fread(P_, sizeof(char), BLOCK_SIZE, F)) == BLOCK_SIZE) {
      fwrite(P_, sizeof(char), BLOCK_SIZE, pipe_fp);
    }
    fwrite(P_, sizeof(char), N, pipe_fp);
    std::cerr << "finished sending ..." << std::endl;
    pclose(pipe_fp);
    fclose(F);
  }
};
void RunServer() {
  std::string server_address("192.168.0.247:50051");
  FileServerImpl<4096> service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cerr << "Server listening on " << server_address << std::endl;
  server->Wait();
}

void RunSpawner() {
  Spawner::Start();
}

int main(int argc, char** argv) {
  // fork and start spawner
  FILE* F;

  int filedes[2];
  if (pipe(filedes) == -1) {
    perror("pipe");
    exit(1);
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  } else if (pid == 0) {
    close(filedes[1]);
    while ((dup2(filedes[0], STDIN_FILENO) == -1) && (errno == EINTR)) {}
    close(filedes[0]);

    RunSpawner();

  } else {
    close(filedes[0]);
    while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
    grpc_init();
    
    RunServer();
    
    grpc_shutdown();
  }

  return 0;
}
