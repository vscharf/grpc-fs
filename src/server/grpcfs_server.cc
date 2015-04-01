#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <memory>
#include <stack>
#include <string>
#include <thread>
#include <mutex>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/server_credentials.h>
#include <grpc++/status.h>
#include <grpc++/stream.h>
#include "src/cpp/server/thread_pool.h"
#include "grpc_fs.pb.h"

#include <boost/interprocess/anonymous_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;
using grpcfs::FileRequest;
using grpcfs::FileRequestAnswer;
using grpcfs::FileServer;

struct RequestQueue {
  static constexpr char SIZE = 5;

  struct Entry {
    char file[256] = "";
    char host[256] = "";
    int port = 0;
  };

  //Mutex to protect access to the queue
  boost::interprocess::interprocess_mutex mutex;
  
  //Condition to wait when the queue is empty
  boost::interprocess::interprocess_condition cond_empty;

  //Condition to wait when the queue is full
  boost::interprocess::interprocess_condition  cond_full;

  bool empty() const {return head_ == tail_; }
  bool full() const {return tail_ == SIZE && head_ == 0; }

  // no synchronisation , no checks ...
  Entry pop() {
    std::cerr << "RequestQueue::pop" << std::endl;
    if(empty()) {
      std::cerr << "RequestQueue::pop empty!" << std::endl;
      assert(false);
    }
    return entries_[head_++];
  }

  void push(Entry E) {
    std::cerr << "RequestQueue::push" << std::endl;
    if(full()) {
      std::cerr << "RequestQueue::push full!" << std::endl;
      assert(false);
    }
    if(tail_ == SIZE) {
      move();
    }
    assert(tail_ < SIZE);
    entries_[tail_++] = E;
  }

private:
  void move() {
    std::cerr << "RequestQueue::move" << std::endl;
    if(head_ == 0 || head_ > tail_) {
      std::cerr << "RequestQueue::move precondition" << std::endl;
      assert(false);
    }
    if(head_ == tail_) {
      head_ = tail_ = 0;
    } else {
      memmove(entries_, entries_ + head_, sizeof(Entry) * (tail_ - head_));
      tail_ -= head_;
      head_ = 0;
    }
  }

  Entry entries_[SIZE];
  char head_ = 0;
  char tail_ = 0;
};

class FileServerImpl final : public FileServer::Service {
public:
  FileServerImpl(RequestQueue& R) : R_(R) {}
  Status GetFile(ServerContext* context, const ::grpcfs::FileRequest* request, ::grpcfs::FileRequestAnswer* response) override
  {
    std::string n = request->name();
    std::string h = request->host();
    int p = request->port();

    if(n.size() >= 255 || h.size() >= 255) {
      std::cerr << "Filename or Hostname too long ..." << std::endl;
      response->set_status(grpcfs::FileRequestAnswer_Status_FAIL);
      return Status(grpc::INVALID_ARGUMENT);
    }

    using namespace boost::interprocess;
    {
      scoped_lock<interprocess_mutex> lock(R_.mutex);
      while(R_.full()) {
        std::cerr << "Queue full ... sleeping" << std::endl;
        R_.cond_full.wait(lock);
      }
      
      std::cerr << "RequestQueue not full" << std::endl;
      RequestQueue::Entry E;
      std::size_t length = n.copy(E.file,255);
      E.file[length]='\0';
      length = h.copy(E.host,255);
      E.host[length]='\0';
      E.port = p;

      R_.push(E);
      R_.cond_empty.notify_one();
    }

    response->set_status(grpcfs::FileRequestAnswer_Status_OK);

    return Status::OK;
  }
private:
  RequestQueue& R_;
};

class Spawner
{
public:
  static void Start(RequestQueue& R) {
    std::cerr << "Start ... " << std::endl;
    std::array<ThreadContext, 5> tctx;
    std::stack<char> free;
    free.push(4);
    free.push(3);
    free.push(2);
    free.push(1);
    free.push(0);

    while(1) {
      if(free.empty()) {
         std::unique_lock<std::mutex> lk(m);
         std::cerr << "All 5 nc threads full ..." << std::endl;
         cv.wait(lk); // don't care about spurious wake-up
      }

      for(char i = 0; i < 5; ++i) {
        if(tctx[i].finished) {
          std::cerr << "join .. " << std::endl;
          tctx[i].t.join();
          tctx[i].finished = false;
          free.push(i);
        }
      }
      if(free.empty()) continue; // in case spurious wake-up happend before

      RequestQueue::Entry E;
      using namespace boost::interprocess;
      {
        scoped_lock<interprocess_mutex> lock(R.mutex);
        while(R.empty()) {
          std::cerr << "Queue empty ... sleeping" << std::endl;
          R.cond_empty.wait(lock);
        }
        std::cerr << "RequestQueue not empty" << std::endl;
        E = R.pop();
        R.cond_full.notify_one();
      }

      char I = free.top();
      free.pop();
      std::cerr << "Spawning thread for " << E.host << ":" << E.port << " " << E.file << std::endl;
      tctx[I].args = {E.file, E.host, E.port};
      tctx[I].t = std::thread(spawn<65536>, std::cref(tctx[I].args), std::ref(tctx[I].finished));
      std::cerr << "... done" << std::endl; 

    }
  }
private:
  struct ThreadContext {
    ThreadContext() { finished = false; }
    std::atomic<bool> finished;
    std::thread t;
    struct ThreadArgs {
      std::string n;
      std::string h;
      int p;
    } args;
  };

  static std::condition_variable cv;
  static std::mutex m;

  template <std::streamsize BLOCK_SIZE>
  static void spawn(const ThreadContext::ThreadArgs& a, std::atomic<bool>& finished) {
    std::cerr << "spawn ... " << a.n << ' ' << a.h << ' ' << a.p << std::endl;
    char P_[BLOCK_SIZE];
    FILE* pipe_fp;
    if((pipe_fp = popen(("nc -v -v -q0 " + a.h + " " + std::to_string(a.p) + " 1>nc.log 2>nc.err").c_str(), "w")) == NULL) {
      perror("popen");
      std::cerr << "popen failed" << std::endl;
      return;
    }

    FILE* F = fopen(a.n.c_str(), "r");
    std::size_t N(0);
    std::cerr << "write ... " << std::endl;
    std::size_t T(0);
    auto start = std::chrono::steady_clock::now();
    while((N = fread(P_, sizeof(char), BLOCK_SIZE, F)) == BLOCK_SIZE) {
      fwrite(P_, sizeof(char), BLOCK_SIZE, pipe_fp);
      T += N;
    }
    fwrite(P_, sizeof(char), N, pipe_fp);
    auto stop = std::chrono::steady_clock::now();
    T += N;
    std::cerr << "finished sending ... " << T/1024./1024./std::chrono::duration<double>(stop - start).count() << " MB/s"  << std::endl;
    pclose(pipe_fp);
    fclose(F);
    finished = true;
    
    std::cerr << "close ... " << std::endl;
    cv.notify_one();
  }
};

std::condition_variable Spawner::cv;
std::mutex Spawner::m;

void RunServer(RequestQueue& R) {
  std::string server_address("192.168.0.247:50051");
  FileServerImpl service(R);

  grpc::ThreadPool thread_pool(2);

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  builder.SetThreadPool(&thread_pool);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cerr << "Server listening on " << server_address << std::endl;
  server->Wait();
}

void RunSpawner(RequestQueue& R) {
  Spawner::Start(R);
}

int main(int argc, char** argv) {
  // create shared memory before fork
  using namespace boost::interprocess;

  //Create an anonymous shared memory segment that contains a RequestQueue
  mapped_region region(anonymous_shared_memory(sizeof(RequestQueue)));
  RequestQueue * R = new (region.get_address()) RequestQueue;

  //The segment is unmapped when "region" goes out of scope

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(1);
  } else if (pid == 0) {
    RunSpawner(*R);
  } else {
    grpc_init();
    
    RunServer(*R);
    
    grpc_shutdown();
  }

  return 0;
}
