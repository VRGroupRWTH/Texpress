#include <texpress/compression/compressor.hpp>
#include <spdlog/spdlog.h>

#include <BC.h>
#include <fp16.h>

namespace texpress {
  void Compressor::listener(const Event& e) {
    if (e.mType == EventType::COMPRESS_BC6H) {
      if (!e.mSendData) {
        spdlog::warn("No file has been uploaded!");
        return;
      }

      if (!e.mReceiveData) {
        spdlog::warn("No data destination provided!");
        return;
      }

      std::vector<char>* inData = static_cast<std::vector<char>*>(e.mSendData);
      std::vector<char>* outData = static_cast<std::vector<char>*>(e.mReceiveData);
      compress(*inData, *outData);
    }
  }

  bool Compressor::compress(const std::vector<char>& input, std::vector<char>& output) {
    output.resize(input.size());
    std::copy(input.begin(), input.end(), output.data());
    spdlog::info("Compressed!");

    return true;
  }
}