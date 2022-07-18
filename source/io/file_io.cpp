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

/*
bool texpress::file_read(const char* path, char* buffer, uint64_t buffer_size, uint64_t element_size, uint64_t physical_offset, uint64_t physical_size, uint64_t src_offset, uint64_t src_stride, uint64_t dest_offset, uint64_t dest_stride, FileType type) {
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
  std::streampos file_end = file.tellg();
  file.seekg(0, std::ios::beg);
  std::streampos file_begin = file.tellg();
  uint64_t file_size = file_end - file_begin;

  // Get dataset locations and size
  file.seekg(physical_offset + physical_size, std::ios::beg);
  std::streampos read_end = file.tellg();
  file.seekg(physical_offset, std::ios::beg);
  std::streampos read_begin = file.tellg();
  uint64_t read_range = read_end - read_begin;

  if (read_range > buffer_size) {
    spdlog::error("Provided buffer for file " + std::string(path) + "is too small: "
      + std::to_string(buffer_size) + "<" + std::to_string(read_range));

    file.close();
    return false;
  }

  if (file_end < read_end) {
    spdlog::error("Provided read range for file " + std::string(path) + "is too small: "
      + std::to_string(file_end) + "<" + std::to_string(read_end));

    file.close();
    return false;
  }

  // Read file
  uint64_t read_invocations = (read_range - src_offset) / src_stride;
  for (uint64_t i = 0; i < read_invocations; i++) {
    file.read(buffer + dest_offset + i * dest_stride, element_size);
    file.seekg(physical_offset + src_offset * element_size + i * src_stride * element_size, std::ios::beg);
  }

  file.close();

  return true;
}
*/

