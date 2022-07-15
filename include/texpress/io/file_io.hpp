#pragma once
#include <cstdint>
#include <vector>
#include <texpress/export.hpp>

namespace texpress {

enum FileType {
  FILE_BINARY = 0,
  FILE_TEXT
};

bool TEXPRESS_EXPORT file_exists(const char* path, FileType type = FileType::FILE_BINARY);
uint64_t TEXPRESS_EXPORT file_size(const char* path, FileType type = FileType::FILE_BINARY);
bool TEXPRESS_EXPORT file_read(const char* path, char* buffer, uint64_t buffer_size, FileType type = FileType::FILE_BINARY);
bool TEXPRESS_EXPORT file_read(const char* path, char* buffer, uint64_t buffer_size, uint64_t character_size, uint64_t character_count = 0, uint64_t offset_src = 0, uint64_t stride_src = 0, uint64_t offset_dst = 0, uint64_t stride_dst = 0);
}
