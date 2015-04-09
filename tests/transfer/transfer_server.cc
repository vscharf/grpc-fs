#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>

#include <boost/asio.hpp>

#include "transfer/transfer.h"

using boost::asio::ip::tcp;

int main(int argc, char *argv[]) {
  if(argc < 2 || argc > 3) {
    std::cerr << "usage: local_transfer_server <PORT> [<OUTPUT>]" << std::endl;
    return 1;
  }

  auto port_num = std::stoul(argv[1]);
  if(port_num < 1024 || port_num > (1<<16)-1) return 2;

  // if no argument is given use std::cin otherwise open the file
  std::basic_ostream<char> *O = &std::cout;
  std::unique_ptr<std::ofstream> F;
  if(argc == 3) {
    F.reset(new std::ofstream(argv[2]));
    O = F.get();
  }

  // parent process
  boost::asio::io_service io_service;
  grpcfs::transfer::Receiver R(io_service, port_num);

  grpcfs::transfer::Receiver::PerformanceData D;

  R.receive(*O, &D);
  io_service.run();

  // wait for receiver to finish
  boost::system::error_code E;
  while(!R.finished(E)) std::this_thread::sleep_for(std::chrono::seconds(1));

  if(E) {
    std::cerr << "Error receiving data: " << E.message() << std::endl;
    return 1;
  } else {
    std::cerr << "Success! Duration = "
              << std::chrono::duration_cast<std::chrono::duration<double>>(D.stop - D.start).count()
              << " s" << std::endl;
  }

  return 0;
}
