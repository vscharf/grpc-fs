#include <fstream>
#include <iostream>
#include <memory>

#include <boost/asio.hpp>

#include "transfer/transfer.h"

using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
  static constexpr unsigned short port_num = 50511;

  if(argc > 2) {
    std::cerr << "usage: local_transfer_server [<OUTPUT>]" << std::endl;
    return 1;
  }

  // if no argument is given use std::cin otherwise open the file
  std::basic_ostream<char> *O = &std::cout;
  std::unique_ptr<std::ofstream> F;
  if(argc == 2) {
    F.reset(new std::ofstream(argv[1]));
    O = F.get();
  }

  // parent process
  boost::asio::io_service io_service;
  grpcfs::transfer::Receiver R(io_service, port_num);

  auto E = R.receive(*O);
  if(E) {
    std::cerr << "Error receiving data: " << E.message() << std::endl;
    return 1;
  } else {
    std::cerr << "Success!" << std::endl;
  }

  return 0;
}
