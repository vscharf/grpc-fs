// -*- C++ -*-
#ifndef GRPCFS_UTIL_TASK_H_
#define GRPCFS_UTIL_TASK_H_

#include <ostream>
#include <string>
#include <boost/system/error_code.hpp>

namespace grpcfs {
namespace util {

struct Task {
  std::string filename;
  std::string hostname;
  unsigned short port;
  boost::system::error_code error;
};

inline std::ostream& operator<<(std::ostream& O, const Task& T) {
  O << "Task: " << T.filename << " -> " << T.hostname << ":" << T.port
    << "  -- ERROR: " << T.error.message();
  return O;
}

inline bool operator==(const Task& L, const Task& R) {
  return L.filename == R.filename && L.hostname == R.hostname && L.port == R.port;
}

} // namespace util
} // namespace grpcfs

#endif //GRPCFS_UTIL_TASK_H_
