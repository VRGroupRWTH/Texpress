#include <texpress/export.hpp>
#include <texpress/io/file_io.hpp>
#include <fstream>
#include <spdlog/spdlog.h>

bool texpress::file_exists(const char* path, FileType type) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::in;

  // Bbinary data
  if (type == FileType::FILE_BINARY) {
    file_mode |= std::ios::binary;
  }

  // Open file
  std::ifstream file;
  file.open(path, file_mode);

  if (!file.is_open()) {
    return false;
  }

  file.close();

  return true;
}

uint64_t texpress::file_size(const char* path, FileType type) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::in | std::ios::ate;

  // Bbinary data
  if (type == FileType::FILE_BINARY) {
    file_mode |= std::ios::binary;
  }

  // Open file
  std::ifstream file;
  file.open(path, file_mode);

  if (!file.is_open()) {
    spdlog::warn("File " + std::string(path) + "could not be opened!");
    return 0;
  }

  // Get file locations and size
  std::streampos end = file.tellg();
  file.seekg(0, std::ios::beg);
  std::streampos begin = file.tellg();
  double file_size = end - begin;

  file.close();

  return file_size;
}

bool texpress::file_read(const char* path, char* buffer, uint64_t buffer_size, FileType type) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::in | std::ios::ate;

  // Bbinary data
  if (type == FileType::FILE_BINARY) {
    file_mode |= std::ios::binary;
  }

  // Open file
  std::ifstream file;
  file.open(path, file_mode);
  
  if (!file.is_open()) {
    spdlog::warn("File " + std::string(path) + "could not be opened!");
    return false;
  }

  // Get file locations and size
  std::streampos end = file.tellg();
  file.seekg(0, std::ios::beg);
  std::streampos begin = file.tellg();
  double file_size = end - begin;

  if (file_size > buffer_size) {
    spdlog::error("Provided buffer for file " + std::string(path) + "is too small: "
      + std::to_string(buffer_size) + "<" + std::to_string(file_size));

    file.close();
    return false;
  }

  // Read file
  if (type == FileType::FILE_BINARY) {
    file.read(buffer, end);
  }
  else {
    file.read(buffer, end);
  }

  file.close();

  return true;
}

bool TEXPRESS_EXPORT texpress::file_read(const char* path, char* buffer, uint64_t buffer_size, uint64_t character_size, uint64_t character_count, uint64_t offset_src, uint64_t stride_src, uint64_t offset_dst, uint64_t stride_dst) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::in | std::ios::ate | std::ios::binary;

  // Open file
  std::ifstream file;
  file.open(path, file_mode);

  if (!file.is_open()) {
    spdlog::warn("File " + std::string(path) + "could not be opened!");
    return false;
  }

  // Get file locations and size
  std::streampos file_end = file.tellg();
  file.seekg(0, std::ios::beg);
  std::streampos file_begin = file.tellg();

  if (offset_src >= (file_begin + file_end)) {
    spdlog::error("Offset for file " + std::string(path) + "is too large.");

    file.close();
    return false;
  }

  file.seekg(offset_src, std::ios::beg);
  std::streampos read_begin = file.tellg();

  if (character_count == 0) {
    character_count = (file_end - read_begin) / character_size;
  }

  if (offset_src + character_size * character_count > file_end) {
    spdlog::error("Reading too many characters of file " + std::string(path) + ".");

    file.close();
    return false;
  }

  file.seekg(offset_src + character_size * character_count, std::ios::beg);
  std::streampos read_end = file.tellg();
  file.seekg(offset_src, std::ios::beg);

  uint64_t read_range = read_end - read_begin;
  uint64_t read_size = read_range / stride_src;
  uint64_t read_characters = read_size / character_size;

  if (read_characters > buffer_size) {
    spdlog::error("Provided buffer for file " + std::string(path) + "is too small: "
      + std::to_string(buffer_size) + "<" + std::to_string(read_size));

    file.close();
    return false;
  }

  // Read file
  for (uint64_t i = 0; i < read_characters; i++) {
    file.read(buffer + offset_dst + stride_dst * character_size * i, character_size);
  }

  file.close();

  return true;
}

