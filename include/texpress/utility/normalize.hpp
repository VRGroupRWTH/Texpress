#pragma once
#include <vector>
#include <glm/vec4.hpp>
#include <texpress/types/texture.hpp>

namespace texpress {
    // Finds the minimum and the maximum of each depth-level in the given dataset.
  // The algorithm infers the shape of the dataset by the given regular grid.
  // Returns a vector with the min/max values in (min, max) order.
    template <typename T>
    static std::vector<float> find_peaks(
        const T* data,
        const glm::ivec4& grid,
        std::size_t vec_len
    )
    {
        // This may not be the actual grid dimension, but this is fine.
        const std::size_t& grid_dim = grid.length();

        const std::size_t  grid_x = (grid_dim >= 1) ? grid[0] : 1;
        const std::size_t  grid_y = (grid_dim >= 2) ? grid[1] : 1;
        const std::size_t  grid_z = (grid_dim >= 3) ? grid[2] : 1;
        const std::size_t  grid_t = (grid_dim == 4) ? grid[3] : 1;

        std::vector<float> peaks(grid_t * grid_z * 2);
        std::size_t        addr_offset = 0;

        for (std::size_t t = 0; t < grid_t; t++)
        {
            for (std::size_t z = 0; z < grid_z; z++)
            {
                float min = static_cast<float>(*(data + addr_offset));
                float max = static_cast<float>(*(data + addr_offset));

                for (std::size_t y = 0; y < grid_y; y++)
                {
                    for (std::size_t x = 0; x < grid_x; x++)
                    {
                        for (std::size_t c = 0; c < vec_len; c++)
                        {
                            const float& val = static_cast<float>(*(data + addr_offset));

                            if (val < min)
                                min = val;

                            else if (val > max)
                                max = val;

                            addr_offset++;
                        }
                    }
                }

                peaks[2 * (t * grid_z + z)] = min;
                peaks[2 * (t * grid_z + z) + 1] = max;
            }
        }

        return peaks;
    }

    static std::vector<float> find_peaks(const Texture& data_container)
    {
        switch (data_container.gl_type) {
        case gl::GL_UNSIGNED_BYTE:
            return find_peaks(data_container.data.data(), data_container.dimensions, data_container.channels);
        case gl::GL_FLOAT:
            return find_peaks((const float*)data_container.data.data(), data_container.dimensions, data_container.channels);
        case gl::GL_DOUBLE:
            return find_peaks((const double*)data_container.data.data(), data_container.dimensions, data_container.channels);
        }

        return {};
    }

    // Finds the minimum and the maximum of each depth-level in the given dataset for each component seperately.
    // The algorithm infers the shape of the dataset by the given regular grid.
    // Returns a vector with the min/max values in (min, max) order.
    template <typename T>
    static std::vector<float> find_peaks_per_component(
        const T* data,
        const glm::ivec4& grid,
        std::size_t               vec_len
    )
    {
        // This may not be the actual grid dimension, but this is fine.
        const std::size_t& grid_dim = grid.length();

        const std::size_t  grid_x = (grid_dim >= 1) ? grid[0] : 1;
        const std::size_t  grid_y = (grid_dim >= 2) ? grid[1] : 1;
        const std::size_t  grid_z = (grid_dim >= 3) ? grid[2] : 1;
        const std::size_t  grid_t = (grid_dim == 4) ? grid[3] : 1;

        std::vector<float> peaks(grid_t * grid_z * vec_len * 2);
        std::size_t        addr_offset = 0;



        for (std::size_t t = 0; t < grid_t; t++)
        {
            for (std::size_t z = 0; z < grid_z; z++)
            {
                std::vector<float> min_c(vec_len);
                std::vector<float> max_c(vec_len);

                for (auto c = 0; c < vec_len; c++)
                {
                    min_c[c] = static_cast<float>(*(data + addr_offset + c));
                    max_c[c] = static_cast<float>(*(data + addr_offset + c));
                }

                for (std::size_t y = 0; y < grid_y; y++)
                {
                    for (std::size_t x = 0; x < grid_x; x++)
                    {
                        for (std::size_t c = 0; c < vec_len; c++)
                        {
                            const float& val = static_cast<float>(*(data + addr_offset));

                            if (val < min_c[c])
                                min_c[c] = val;

                            else if (val > max_c[c])
                                max_c[c] = val;

                            addr_offset++;
                        }
                    }
                }

                for (auto c = 0; c < vec_len; c++)
                {
                    peaks[2 * (t * grid_z * vec_len + z * vec_len + c)] = min_c[c];
                    peaks[2 * (t * grid_z * vec_len + z * vec_len + c) + 1] = max_c[c];
                }
            }
        }

        return peaks;
    }

    // Finds the minimum and the maximum of each depth-level per component in the given dataset.
    // The algorithm infers the shape of the dataset by the given regular grid.
    // Returns a vector with the min/max values in (min, max) order.
    template<typename T, int i>
    static std::vector<float> find_peaks_per_component(
        const glm::vec<i, T>* data,
        const std::vector<std::size_t>& grid,
        std::size_t               vec_len
    )
    {
        std::size_t max_components = std::max(i, vec_len);
        return find_peaks_per_component(static_cast<T*>(data), grid, max_components);
    }


    template<typename T>
    static std::vector<float> find_peaks_per_component(const Texture& data_container)
    {
        switch (data_container.gl_type) {
        case gl::GL_UNSIGNED_BYTE:
            return find_peaks_per_component(data_container.data.data(), data_container.dimensions, data_container.channels);
        case gl::GL_FLOAT:
            return find_peaks_per_component((const float*)data_container.data.data(), data_container.dimensions, data_container.channels);
        case gl::GL_DOUBLE:
            return find_peaks_per_component((const double*)data_container.data.data(), data_container.dimensions, data_container.channels);
        }
        return {};
    }


    // Convenience function to get the correct normalization parameters.
    // Returns safe normalization parameters: 
    template <typename T>
    static float normalize_val(T value, const std::vector<float>& peaks, std::size_t depth_level)
    {
        float min = peaks[2 * depth_level];
        float max = peaks[2 * depth_level + 1];

        // Valid
        if (min != max)
            return (static_cast<float>(value) - min) / (max - min);

        // Don't divide by 0
        if (min == 0)
            return static_cast<float>(value);

        // Still don't divide by 0, but normalize if you can (doesn't matter if you choose division by max or min)
        return static_cast<float>(value) / (max);
    }

    // Convenience function to get the correct normalization parameters.
   // Returns safe normalization parameters: 
    template <typename T>
    static float normalize_val_per_component(T value, int vec_len, const std::vector<float>& peaks, std::size_t depth_level, int component)
    {
        float min = peaks[2 * (depth_level * vec_len + component)];
        float max = peaks[2 * (depth_level * vec_len + component) + 1];

        // Valid
        if (min != max)
            return (static_cast<float>(value) - min) / (max - min);

        // Don't divide by 0
        if (min == 0)
            return static_cast<float>(value);

        // Still don't divide by 0, but normalize if you can (doesn't matter if you choose division by max or min)
        return static_cast<float>(value) / (max);
    }

    template <typename T>
    static float normalize_val_per_volume(T value, int vec_len, const glm::vec3& minimum, const glm::vec3& maximum, int component)
    {
        float min = minimum[component];
        float max = maximum[component];

        // Valid
        if (min != max)
            return (static_cast<float>(value) - min) / (max - min);

        // Don't divide by 0
        if (min == 0)
            return static_cast<float>(value);

        // Still don't divide by 0, but normalize if you can (doesn't matter if you choose division by max or min)
        return static_cast<float>(value) / (max);
    }


    static float denormalize_float(float value, const std::vector<float>& peaks, std::size_t depth_level)
    {
        float min = peaks[2 * depth_level];
        float max = peaks[2 * depth_level + 1];

        // Valid
        if (min != max)
            return value * (max - min) + min;

        // Edge case: min = max = 0
        if (min == 0)
            return value;

        // Edge case: min = max
        return value * (max);
    }

    template <typename T>
    static T denormalize_per_component(T value, int vec_len, const std::vector<T>& peaks, std::size_t depth_level, int component)
    {
        float min = peaks[2 * (depth_level * vec_len + component)];
        float max = peaks[2 * (depth_level * vec_len + component) + 1];

        // Valid
        if (min != max)
            return value * (max - min) + min;

        // Edge case: min = max = 0
        if (min == 0)
            return value;

        // Edge case: min = max
        return value * (max);
    }

    template <typename T>
    static T denormalize_per_volume(T value, int vec_len, const glm::vec3& minimum, const glm::vec3& maximum, int component)
    {
        float min = minimum[component];
        float max = maximum[component];

        // Valid
        if (min != max)
            return value * (max - min) + min;

        // Edge case: min = max = 0
        if (min == 0)
            return value;

        // Edge case: min = max
        return value * (max);
    }
}