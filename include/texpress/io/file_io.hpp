#pragma once
#include <cstdint>
#include <vector>

namespace texpress {

    enum FileType {
        FILE_BINARY = 0,
        FILE_TEXT
    };

    bool file_exists(const char* path);
    uint64_t file_size(const char* path);
    bool file_read(const char* path, char* buffer, uint64_t buffer_size, uint64_t offset = 0, FileType type = FileType::FILE_BINARY);
    bool file_save(const char* path, char* buffer, uint64_t buffer_size, bool append = false, FileType type = FileType::FILE_BINARY);

    //bool  file_read(const char* path, char* buffer, uint64_t buffer_size, uint64_t element_size, uint64_t physical_offset, uint64_t physical_size, uint64_t src_offset = 0, uint64_t src_stride = 1, uint64_t dest_offset = 0, uint64_t dest_stride = 1, FileType type = FileType::FILE_BINARY);
}