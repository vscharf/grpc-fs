#include <iostream>
#include <fstream>
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

#include <boost/asio.hpp>

#include <sys/types.h>
#include <unistd.h>

#include "transfer/transfer.h"

using grpc::ChannelArguments;
using grpc::ChannelInterface;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;
using grpcfs::FileServer;
using grpcfs::FileRequest;
using grpcfs::FileRequestAnswer;

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

    pid_t pid;
    unsigned short port = getpid() % (65536 - 30000) + 30000;

    std::ofstream F(out_filename);

    boost::asio::io_service io_service;
    grpcfs::transfer::Receiver R(io_service, port);
    R.receive(F); // async handler

    FileRequest r;
    r.set_name(filename);
    r.set_host(get_hostname());
    r.set_port(port);

    make_request(r, io_service);
    
    std::cout << "Waiting for file!" << std::endl;

    // wait for receiver to finish
    boost::system::error_code E;
    while(!io_service.stopped() && !R.finished(E)) io_service.run_one();

    if(E) {
      std::cout << "Error receiving data: " << E.message() << std::endl;
      return false;
    }

    return true;
  }
  void Shutdown() { stub_.reset(); }

private:
  std::unique_ptr<FileServer::Stub> stub_;

  void make_request(const FileRequest& R, boost::asio::io_service& io_service) {
    std::cout << "Requesting file " << R.name() << " for port " << R.port() << std::endl;

    ClientContext context;
    FileRequestAnswer c;
    Status status = stub_->GetFile(&context, R, &c);

    if (!status.IsOk() || c.status() != grpcfs::FileRequestAnswer_Status_OK) {
      std::cout << "RPC failed" << std::endl;
      io_service.stop();
    } else {
      std::cout << "RPC sucessfull" << std::endl;
    }
  }
};

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Need 2 arguments!\n";
    return 1;
  }
  grpc_init();

  FileServerClient client(grpc::CreateChannel("192.168.0.247:50051", grpc::InsecureCredentials(), ChannelArguments()));

  std::cout << "-------------- GetFile --------------" << std::endl;
  client.GetFile(argv[1], argv[2]);

  client.Shutdown();

  grpc_shutdown();
}
