#pragma once

#include <chrono>

struct cpu_timer {
 public:
  cpu_timer() : start_time(), end_time() {}
  void start() { start_time = std::chrono::high_resolution_clock::now(); }
  void stop() { end_time = std::chrono::high_resolution_clock::now(); }
  double elapsed_seconds() const {
    std::chrono::duration<double> elapsed_seconds = end_time - start_time;
    return elapsed_seconds.count();
  }

  template <typename T>
  double compute_gbs(T count) const {
    auto seconds = elapsed_seconds();
    auto giga = double{count} / double{1u << 30};
    auto gbs = giga / seconds;
    return gbs;
  }

 private:
  std::chrono::high_resolution_clock::time_point start_time;
  std::chrono::high_resolution_clock::time_point end_time;
};
