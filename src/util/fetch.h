// -*- C++ -*-
#ifndef GRPCFS_UTIL_FETCH_H_
#define GRPCFS_UTIL_FETCH_H_

#include <memory>
#include <tbb/concurrent_queue.h>
#include "task.h"

namespace grpcfs {
namespace util {

// pool of threads that fetch files from disk to ram
class FetchPool
{
public:
  FetchPool();
  virtual ~FetchPool(); // unique_ptr type must be known in destructor
  void run(std::size_t N,
           tbb::concurrent_bounded_queue<Task>& fetch,
           tbb::concurrent_bounded_queue<Task>& transfer);
  void shutdown();
private:
  class Impl;
  std::unique_ptr<Impl> pImpl_;
}; // class FetchPool

} // namespace util
} // namespace grpcfs

#endif // GRPCFS_UTIL_FETCH_H_
