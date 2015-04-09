#include "pipeline.h"

#include <iostream>

using namespace grpcfs::util;

namespace {
  void log_completed(tbb::concurrent_bounded_queue<Task>& C) {
    using std::cerr;
    using std::endl;
    for(;;) {
      Task T;
      C.pop(T);
      
      // empty Task is sentry object for abort
      if(T == Task()) break;

      if(T.error) {
        cerr << "FAILED:  ";
      } else {
        cerr << "SUCCESS: ";
      }
      cerr << T << endl;
    }
    cerr << "Logger received sentry object!" << endl;
  }
} // anonymous namespace

Pipeline::Pipeline(std::size_t n_fetch, std::size_t n_transfer)
  : n_fetch_(n_fetch)
  , n_transfer_(n_transfer)
{
  // bound the transfer queue to avoid running out-of-memory
  transfer_.set_capacity(n_transfer);

  // start fetcher
  fetcher_.run(n_fetch, fetch_, transfer_);

  // start transferer
  transferer_.run(n_transfer, transfer_, complete_);

  // start logging thread
  logger_ = std::thread(log_completed, std::ref(complete_));
}

void Pipeline::shutdown() {
  // push sentry objects
  for(std::size_t i = 0; i < n_fetch_ + 1; ++i) {
    fetch_.push(Task{});
  }

  fetcher_.shutdown();

  // push sentry objects
  for(std::size_t i = 0; i < n_transfer_ + 1; ++i) {
    transfer_.push(Task{});
  }
  transferer_.shutdown();

  complete_.push(Task{});
  logger_.join();
}

Pipeline::~Pipeline() {
  // empty fetch_ queue
  fetch_.clear();
  
  // empty transfer_ queue
  transfer_.clear();
  
  shutdown();
}
