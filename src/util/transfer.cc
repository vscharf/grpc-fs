#include "transfer.h"

#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "transfer/transfer.h"

using namespace grpcfs::util;

namespace {
  void do_transfer(Task& task) {
    std::cerr << "do_transfer" << std::endl;
    if(task.error) return; // if an error occuered earlier don't try to transfer
    
    std::ifstream F(task.filename);
    task.error = grpcfs::transfer::Sender::send(task.hostname, task.port, F);
    std::cerr << "do_transfer finished" << std::endl;
  }
} // anonymous namespace

class TransferPool::Impl {
public:
  void run(std::size_t N,
           tbb::concurrent_bounded_queue<Task>& transfer,
           tbb::concurrent_bounded_queue<Task>& complete)
  {
    while(N--) {
      t_.push_back(std::thread(&grpcfs::util::TransferPool::Impl::thread_run, this,
                               std::ref(transfer), std::ref(complete)));
    }
  };
  void shutdown() {
    for(auto& T : t_) {
      if(T.joinable()) T.join();
    }
  }

  virtual ~Impl() {
    shutdown();
  }

private:
  std::vector<std::thread> t_;

  void thread_run(tbb::concurrent_bounded_queue<Task>& transfer,
                  tbb::concurrent_bounded_queue<Task>& complete)
  {
    for(;;) {
      Task T;
      transfer.pop(T);

      // empty Task Object serves as abort sentry
      if(T == Task()) break;
      
      do_transfer(T);
      complete.push(T);
    }
  }
};

TransferPool::TransferPool() : pImpl_(new Impl) {}
TransferPool::~TransferPool() {}

void TransferPool::run(std::size_t N,
                       tbb::concurrent_bounded_queue<Task>& transfer,
                       tbb::concurrent_bounded_queue<Task>& complete)
{
  pImpl_->run(N, transfer, complete);
}

void TransferPool::shutdown() {
  pImpl_->shutdown();
}
