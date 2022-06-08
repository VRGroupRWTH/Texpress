#pragma once
#include <cstdint>
#include <vector>

namespace texpress {

enum FileType {
  FILE_BINARY = 0,
  FILE_TEXT
};

bool TEXPRESS_EXPORT file_exists(const char* path, FileType type = FileType::FILE_BINARY);
uint64_t TEXPRESS_EXPORT file_size(const char* path, FileType type = FileType::FILE_BINARY);
bool TEXPRESS_EXPORT file_read(const char* path, char* buffer, uint64_t buffer_size, FileType type = FileType::FILE_BINARY);
}
