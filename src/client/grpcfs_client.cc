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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

using grpc::ChannelArguments;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using grpcfs::FileServer;
using grpcfs::FileRequest;
using grpcfs::FileRequestAnswer;

pid_t spawn_netcat(int port, std::string outfile) {
  pid_t pid = fork();

  if(pid) {
    // parent process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if(waitpid(pid, nullptr, WNOHANG)) return -1;
    return pid;
  } else {
    int fd = open(outfile.c_str(), O_WRONLY|O_CREAT, 0666);
    if(fd < 0) return -1;
    dup2(fd, 1);
    close(fd);
    if(execlp("nc", "nc", "-l", std::to_string(port).c_str(), NULL)) return -1;
  }
}

std::string get_hostname() {
  char n[1024];
  n[1023] = '\0';
  gethostname(n, 1023);
  return std::string(n);
}

class FileServerClient {
public:
  FileServerClient(std::shared_ptr<ChannelInterface> channel)
    : stub_(FileServer::NewStub(channel)) {}

  bool GetFile(std::string filename, std::string out_filename) {
    ClientContext context;

    // try to open nc listening on some port for 10 times then bail out
    pid_t pid;
    int port = getpid() % (65536 - 30000) + 30000;

    std::size_t N(10);
    while(N--) {
      if((pid = spawn_netcat(port, out_filename)) > 0) break;
      port = rand() % (65536 - 30000) + 30000;
    }      
    if(!N) {
      std::cout << "Couldn't spawn netcat\n";
      return false;
    }

    FileRequest r;
    r.set_name(filename);
    r.set_host(get_hostname());
    r.set_port(port);
    std::cout << "Requesting file " << r.name() << " for port " << port << "\n";

    FileRequestAnswer c;
    Status status = stub_->GetFile(&context, r, &c);

    if (!status.IsOk()) {
      std::cout << "RPC failed\n";
      return false;
    } else {
      if(c.status() != grpcfs::FileRequestAnswer_Status_OK) {
        std::cout << "Status failed\n";
        return false;
      }
    }
    
    // wait for netcat to finish
    if(waitpid(pid, nullptr, 0) < 0) {
      std::cout << "Error waiting for netcat ... \n";
      kill(pid, SIGTERM);
    }

    return true;
  }
  void Shutdown() { stub_.reset(); }

private:
  std::unique_ptr<FileServer::Stub> stub_;
};

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Need 2 arguments!\n";
    return 1;
  }
  grpc_init();

  FileServerClient client(grpc::CreateChannel("192.168.0.247:50051", grpc::InsecureCredentials(), ChannelArguments()));

  std::cout << "-------------- GetFile --------------\n";
  client.GetFile(argv[1], argv[2]);

  client.Shutdown();

  grpc_shutdown();
}
