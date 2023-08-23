#include <texpress/utility/stringtools.hpp>

namespace texpress
{
    std::string str_lowercase(const char* input) {
        std::string out;
        out.reserve(strlen(input));
        for (int i = 0; i < strlen(input); i++) {
            out.push_back(tolower(input[i]));
        }
        return out;
    }

    std::string str_lowercase(const std::string& input) {
        std::string out;
        out.reserve(input.length());
        for (int i = 0; i < input.length(); i++) {
            out.push_back(tolower(input[i]));
        }
        return out;
    }

    std::string str_canonical(const char* input, uint64_t depth_level, uint64_t time_level) {
        std::string out(str_lowercase(input));

        if (depth_level != uint64_t(-1)) {
            out.push_back('_');
            out.append(std::to_string(depth_level));
        }
        if (time_level != uint64_t(-1)) {
            out.push_back('_');
            out.append(std::to_string(time_level));
        }

        return out;
    }

    std::string str_canonical(const std::string& input, uint64_t depth_level, uint64_t time_level) {
        std::string out(str_lowercase(input));

        if (depth_level != uint64_t(-1)) {
            out.push_back('_');
            out.append(std::to_string(depth_level));
        }
        if (time_level != uint64_t(-1)) {
            out.push_back('_');
            out.append(std::to_string(time_level));
        }

        return out;
    }
}
