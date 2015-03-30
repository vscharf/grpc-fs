#include <chrono>
#include <iostream>
#include <memory>

using namespace std;
using namespace std::chrono;

int main(int, char*[]) {
  constexpr streamsize count = 4096;
  streamsize payload = 0;
  char b[4096];
  
  
  cin.peek();
  auto start = steady_clock::now();
  while(cin.read(b, count)) {
    payload += count;
  }
  payload += cin.gcount();
  auto stop = steady_clock::now();
  
  std::cout << "Result: time: " << duration<double, std::milli>(stop - start).count() << " ms" << std::endl;
  std::cout << "Payload: " << payload/1024./1024. << " MB" << std::endl;
  std::cout << "Rate:   " << payload/1024./1024./duration<double>(stop - start).count() << " MB/s" << std::endl;

  return 0;
}
