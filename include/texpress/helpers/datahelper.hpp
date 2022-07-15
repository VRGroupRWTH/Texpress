#pragma once
#include <vector>

namespace texpress {
  template <typename T>
  void interleave(std::vector<T> data, uint64_t chunks, uint64_t chunk_size) {
    std::vector<T> temp(chunks);

    for (uint64_t i = 0; i < chunk_size; i++) {
      for (uint64_t chunk = 0; chunk < chunks; chunk++) {
        temp[chunk] = data[chunk * chunk_size + i];

      }

    }
  }
}