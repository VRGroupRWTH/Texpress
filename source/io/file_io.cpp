#include <texpress/export.hpp>
#include <texpress/io/file_io.hpp>
#include <fstream>
#include <spdlog/spdlog.h>

bool texpress::file_exists(const char* path) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::in;

  // Open file
  std::ifstream file;
  file.open(path, file_mode);

  if (!file.is_open()) {
    return false;
  }

  file.close();

  return true;
}

uint64_t texpress::file_size(const char* path) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::in | std::ios::ate;

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
bool texpress::file_save(const char* path, char* buffer, uint64_t buffer_size, FileType type) {
  // Default: open file at the end of file
  std::ios::ios_base::openmode file_mode = std::ios::out;

  // Bbinary data
  if (type == FileType::FILE_BINARY) {
    file_mode |= std::ios::binary;
  }

  // Open file
  std::ofstream file(path, file_mode);
  if (!file) {
    spdlog::warn("File " + std::string(path) + "could not be opened!");
    return false;
  }

  // Get file locations and size
  file.write(buffer, buffer_size);
  file.close();

  return file.good();
}

