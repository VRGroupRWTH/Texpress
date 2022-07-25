#pragma once
#include <string>

namespace texpress
{
  std::string str_lowercase(const char* input);
  std::string str_lowercase(const std::string& input);

  std::string str_canonical(const char* input, uint64_t depth_level = -1, uint64_t time_level = -1);
  std::string str_canonical(const std::string& input, uint64_t depth_level = -1, uint64_t time_level = -1);
  }
