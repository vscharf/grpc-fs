// -*- C++ -*-
#ifndef GRPCFS_UTIL_TRANSFER_H_
#define GRPCFS_UTIL_TRANSFER_H_

#include <memory>
#include <tbb/concurrent_queue.h>
#include "task.h"

namespace grpcfs {
namespace util {

// pool of threads that transfer files to the targe
class TransferPool
{
public:
  TransferPool();
  virtual ~TransferPool(); // unique_ptr type must be known in destructor
  void run(std::size_t N,
           tbb::concurrent_bounded_queue<Task>& transfer,
           tbb::concurrent_bounded_queue<Task>& complete);
  void shutdown();

private:
  class Impl;
  std::unique_ptr<Impl> pImpl_;
}; // class TransfererPool

} // namespace util
} // namespace grpcfs

#endif // GRPCFS_UTIL_TRANSFER_H_
