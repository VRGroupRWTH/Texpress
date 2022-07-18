#pragma once
#include <vector>
#include <thread>

namespace texpress {
  template <typename T>
  std::vector<T> interleave(const std::vector<T>& data, uint64_t chunks, uint64_t chunk_size) {
    std::vector<T> out(data.size());

    // Helper func
    auto func = [&out, &data, chunks, chunk_size](uint64_t chunk) {
      for (uint64_t i = 0; i < chunk_size; i++) {
        out[chunk + i * chunks] = data[chunk * chunk_size + i];
      }
    };

    // Multithreaded interleaving
    std::vector<std::thread> threads;
    for (uint64_t chunk = 0; chunk < chunks; chunk++) {
      threads.push_back(std::thread(func, chunk));
    }

    for (auto& thread : threads) {
      if (thread.joinable())
        thread.join();
    }
    

    /* Single threaded
     * 
    for (uint64_t chunk = 0; chunk < chunks; chunk++) {
      for (uint64_t i = 0; i < chunk_size; i++) {
        out[chunk + i * chunks] = data[chunk * chunk_size + i];
      }
    }
    */

    return out;
  }

  template <typename T>
  std::vector<T> interleave_force(const std::vector<T>& data, uint64_t chunks, uint64_t chunk_size, const T& dummy_val) {
    std::vector<T> out(chunks * chunk_size);

    // Helper func
    auto func = [&out, &data, &dummy_val, chunks, chunk_size](uint64_t chunk) {
      for (uint64_t i = 0; i < chunk_size; i++) {
        uint64_t src_id = chunk * chunk_size + i;
        uint64_t dest_id = chunk + i * chunks;

        if (src_id < data.size())
          out[dest_id] = data[src_id];
        else
          out[dest_id] = dummy_val;
      }
    };

    // Multithreaded interleaving
    std::vector<std::thread> threads;
    for (uint64_t chunk = 0; chunk < chunks; chunk++) {
      threads.push_back(std::thread(func, chunk));
    }

    for (auto& thread : threads) {
      if (thread.joinable())
        thread.join();
    }

    return out;
  }
}