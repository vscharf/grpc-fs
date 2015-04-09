#include "fetch.h"

#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace grpcfs::util;

namespace {
  void do_fetch(Task& task) {
    // TODO check if file exists
    std::ifstream I(task.filename);
    std::ofstream O("/dev/null");
    O << I.rdbuf();
  }
} // anonymous namespace

class FetchPool::Impl {
public:
  void run(std::size_t N,
           tbb::concurrent_bounded_queue<Task>& fetch,
           tbb::concurrent_bounded_queue<Task>& transfer)
  {
    while(N--) {
      t_.push_back(std::thread(&grpcfs::util::FetchPool::Impl::thread_run, this,
                               std::ref(fetch), std::ref(transfer)));
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

  void thread_run(tbb::concurrent_bounded_queue<Task>& fetch,
                  tbb::concurrent_bounded_queue<Task>& transfer)
  {
    for(;;) {
      Task T;
      fetch.pop(T);

      // empty Task Object serves as abort sentry
      if(T == Task()) break;

      do_fetch(T);
      transfer.push(T);
    }
  }
};

FetchPool::FetchPool() : pImpl_(new Impl) {}
FetchPool::~FetchPool() {}

void FetchPool::run(std::size_t N,
                      tbb::concurrent_bounded_queue<Task>& fetch,
                      tbb::concurrent_bounded_queue<Task>& transfer)
{
  pImpl_->run(N, fetch, transfer);
}

void FetchPool::shutdown() {
  pImpl_->shutdown();
}
