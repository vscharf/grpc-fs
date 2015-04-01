#include <fstream>
#include <iostream>

#include <boost/asio.hpp>

#include "transfer/transfer.h"

using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
  static constexpr unsigned short port_num = 50511;

  if(argc > 2) {
    std::cerr << "usage: local_transfer [<INPUT>]" << std::endl;
    return 1;
  }

  // if no argument is given use std::cin otherwise open the file
  std::basic_istream<char> *O = &std::cin;
  std::unique_ptr<std::ifstream> F;
  if(argc == 2) {
    F.reset(new std::ifstream(argv[1]));
    O = F.get();
  }

  auto E = grpcfs::transfer::Sender::send("localhost", port_num, *O);

  if(E) {
    std::cerr << "Error sending data: " << E.message() << std::endl;
    return 1;
  } else {
    std::cout << "Success!" << std::endl;
  }

  return 0;
}
