#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

#include <boost/multi_array.hpp>
#include <glm/glm.hpp>

#include <texpress/utility/permute_for.hpp>

namespace texpress
{
    template <typename _element_type, std::size_t _dimensions>
    struct regular_grid
    {
        using element_type = _element_type;
        using domain_type = glm::vec<_dimensions, float>;
        using index_type = glm::vec<_dimensions, std::size_t>;
        using container_type = boost::multi_array<element_type, _dimensions>;

        static constexpr std::size_t dimensions = _dimensions;

        bool          contains(const domain_type& position) const
        {
            for (std::size_t i = 0; i < dimensions; ++i)
            {
                const auto subscript = std::floor((position[i] - offset[i]) / spacing[i]);
                if (std::int64_t(0) > std::int64_t(subscript) || std::size_t(subscript) >= data.shape()[i] - 1)
                    return false;
            }
            return true;
        }
        element_type  interpolate(const domain_type& position) const
        {
            domain_type weights;
            index_type  start_index;
            index_type  end_index;
            index_type  increment;

            for (std::size_t i = 0; i < dimensions; ++i)
            {
                weights[i] = std::fmod((position[i] - offset[i]), spacing[i]) / spacing[i];
                start_index[i] = std::floor((position[i] - offset[i]) / spacing[i]);
                end_index[i] = start_index[i] + 2;
                increment[i] = 1;
            }

            std::vector<element_type> intermediates;
            intermediates.reserve(std::pow(2, dimensions));
            permute_for<index_type>([&](const index_type& index)
                {
                    intermediates.push_back(data(*reinterpret_cast<const std::array<std::size_t, 3>*>(&index)));
                }, start_index, end_index, increment);

            for (std::int64_t i = dimensions - 1; i >= 0; --i)
                for (std::size_t j = 0; j < std::pow(2, i); ++j)
                    intermediates[j] = (1.0 - weights[i]) * intermediates[2 * j] + weights[i] * intermediates[2 * j + 1];
            return intermediates[0];
        }

        container_type data{};
        domain_type    offset{};
        domain_type    size{};
        domain_type    spacing{};
    };

    typedef regular_grid<float, 2> fgrid2;
    typedef regular_grid<float, 3> fgrid3;
    typedef regular_grid<float, 4> fgrid4;

    typedef regular_grid<double, 2> dgrid2;
    typedef regular_grid<double, 3> dgrid3;
    typedef regular_grid<double, 4> dgrid4;

    typedef fgrid2 grid2;
    typedef fgrid3 grid3;
    typedef fgrid4 grid4;
}
