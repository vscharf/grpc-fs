// -*- C++ -*-
#include <string>
#include <boost/asio/ip/tcp.hpp>

namespace grpcfs {
namespace transfer {

class Receiver {
public:
  Receiver(boost::asio::io_service& io_service, unsigned short port_num)
    : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_num))
  {}

  template<class ostream>
  boost::system::error_code receive(ostream& stream);

private:
  boost::asio::ip::tcp::acceptor acceptor_;
}; // class Receiver

class Sender {
public:
  template<class istream>
  static boost::system::error_code send(std::string host, unsigned short port_num, istream& stream);
}; // class Sender

template<class ostream>
inline boost::system::error_code Receiver::receive(ostream& out_stream)
{
  using boost::asio::ip::tcp;

  tcp::iostream stream;
  boost::system::error_code ec;
  acceptor_.accept(*stream.rdbuf(), ec);
  if(ec) return ec;

  out_stream << stream.rdbuf();

  // basic_ostream::operator bool() return true for eof and good
  if(out_stream && stream.error() == boost::asio::error::eof) return ec;
  return stream.error();
}

template<class istream>
inline boost::system::error_code Sender::send(std::string host, unsigned short port_num, istream& in_stream)
{
  using boost::asio::ip::tcp;
  tcp::iostream stream(host, std::to_string(port_num));
  if(!stream) {
    return stream.error();
  }

  in_stream >> stream.rdbuf();

  if(!in_stream.eof() || stream.error()) return stream.error();

  return boost::system::error_code();
}

} // namespace grpcfs::transfer
} // namespace grpcfs