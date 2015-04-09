// -*- C++ -*-
#ifndef GRPCFS_TRANSFER_TRANSFER_H_
#define GRPCFS_TRANSFER_TRANSFER_H_

#include <atomic>
#include <chrono>
#include <ios> // for std::streamsize
#include <iostream>
#include <string>
#include <memory>
#include <boost/asio/ip/tcp.hpp>

namespace grpcfs {
namespace transfer {

class Receiver {
public:
  struct PerformanceData {
    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point stop;
    std::streamsize transferred;
  };

  Receiver(boost::asio::io_service& io_service, unsigned short port_num)
    : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_num))
    , finished_(false)
  {}

  template<class ostream>
  void receive(ostream& stream, PerformanceData* p = nullptr);

  bool finished(boost::system::error_code& ec) {
    ec = ec_;
    return finished_;
  }

  void close() {
    acceptor_.close();
  }

private:
  boost::asio::ip::tcp::acceptor acceptor_;

  std::atomic<bool> finished_;
  boost::system::error_code ec_;

  template<class ostream>
  struct Handler {
    Handler(boost::asio::ip::tcp::iostream& S, ostream& O, Receiver& R, PerformanceData* P = nullptr)
      : S_(S), O_(O), R_(R), P_(P) {}
    void operator()(const boost::system::error_code& ec) {
      std::cerr << "Receive Handler: " << ec.message() << std::endl;
      if(ec) {
        R_.ec_ = ec;
        R_.finished_ = true;
      }
      auto start = std::chrono::steady_clock::now();
      O_ << S_.rdbuf();
      auto stop =  std::chrono::steady_clock::now();
      if(P_) {
        *P_ = {start, stop, S_.gcount()};
      }

      std::cerr << "Receive Handler finished: " << S_.error().message() << std::endl;      

      // basic_ostream::operator bool() return true for eof and good
      if(O_ && S_.error() == boost::asio::error::eof) R_.ec_ = ec;
      else R_.ec_  = S_.error();
      R_.finished_ = true;
    }
  private:
    boost::asio::ip::tcp::iostream& S_;
    ostream& O_;
    Receiver& R_;
    PerformanceData* P_;
  };

  boost::asio::ip::tcp::iostream stream;
}; // class Receiver

class Sender {
public:
  template<class istream>
  static boost::system::error_code send(std::string host, unsigned short port_num, istream& stream);
}; // class Sender

template<class ostream>
inline void Receiver::receive(ostream& out_stream, PerformanceData* p /*= nullptr*/)
{
  using boost::asio::ip::tcp;

  Handler<ostream> H(stream, out_stream, *this, p);
  acceptor_.async_accept(*stream.rdbuf(), H);
}

template<class istream>
inline boost::system::error_code Sender::send(std::string host, unsigned short port_num, istream& in_stream)
{
  std::cerr << "send" << std::endl;
  using boost::asio::ip::tcp;
  tcp::iostream stream(host, std::to_string(port_num));
  if(!stream) {
    std::cerr << "send error: " << stream.error().message() << std::endl;
    return stream.error();
  }

  in_stream >> stream.rdbuf();

  std::cerr << "send complete: " << stream.error().message() << std::endl;

  if(!in_stream.eof() || stream.error()) return stream.error();

  return boost::system::error_code();
}

} // namespace grpcfs::transfer
} // namespace grpcfs

#endif // GRPCFS_TRANSFER_TRANSFER_H_
