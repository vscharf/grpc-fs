// -*- C++ -*-
#ifndef GRPCFS_UTIL_PIPELINE_H
#define GRPCFS_UTIL_PIPELINE_H

#include <string>
#include <thread>
#include <tbb/concurrent_queue.h>
#include "task.h"
#include "fetch.h"
#include "transfer.h"

namespace grpcfs {
namespace util {

class Pipeline {
public:
  Pipeline(std::size_t n_fetch, std::size_t n_transfer);
  Pipeline(const Pipeline&) = delete;
  Pipeline(Pipeline&&) = delete;
  Pipeline& operator=(const Pipeline&) = delete;
  Pipeline& operator=(Pipeline&&) = delete;
  virtual ~Pipeline();

  bool push(std::string filename, std::string hostname, unsigned short port) {
    return fetch_.try_push(Task{filename, hostname, port});
  }
  void shutdown();

private:
  tbb::concurrent_bounded_queue<Task> fetch_;
  tbb::concurrent_bounded_queue<Task> transfer_;
  tbb::concurrent_bounded_queue<Task> complete_;

  std::size_t n_fetch_;
  std::size_t n_transfer_;

  FetchPool fetcher_;
  TransferPool transferer_;

  std::thread logger_; // logs 
};

} // namespace util
} // namespace grpcfs

#endif //GRPCFS_UTIL_PIPELINE_H
