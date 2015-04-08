#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include "transfer/transfer.h"

using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
  unsigned short port_num = 50511;

  if(argc > 3 || argc < 2) {
    std::cerr << "usage: local_transfer <HOST>[:<PORT>] [<INPUT>]" << std::endl;
    return 1;
  }
  
  using std::begin;
  using std::end;
  std::string host = argv[1];
  auto I = std::find(begin(host), end(host), ':') - begin(host);
  if(I != host.size()) {
    port_num = std::stoull(host.substr(I+1));
    host = host.substr(0, I); 
  }

  // if no argument is given use std::cin otherwise open the file
  std::basic_istream<char> *O = &std::cin;
  std::unique_ptr<std::ifstream> F;
  if(argc == 3) {
    F.reset(new std::ifstream(argv[2]));
    O = F.get();

    // read the file once to /dev/zero so that it is cached
    std::ofstream N("/dev/null");
    N << O->rdbuf();
    O->seekg(0); // rewind
  }

  auto E = grpcfs::transfer::Sender::send(host, port_num, *O);

  if(E) {
    std::cerr << "Error sending data: " << E.message() << std::endl;
    return 1;
  } else {
    std::cout << "Success!" << std::endl;
  }

  return 0;
}
