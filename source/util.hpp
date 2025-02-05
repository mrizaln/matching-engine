#pragma once

#include <vector>
#include <span>

namespace match_engine::util
{
    /**
     * @brief Delete elements from a vector by their indices (sorted in ascending order).
     *
     * @tparam T The type of the elements in the vector.
     * @param vector The vector to delete elements from.
     * @param indices The indices of the elements to delete (sorted in ascending order).
     * @return The number of elements deleted.
     */
    template <typename T>
    void erase_by_indices(std::vector<T>& vector, std::span<const std::size_t> indices)
    {
        if (indices.empty()) {
            return;
        }

        using diff = typename std::vector<T>::difference_type;

        auto write = vector.begin() + static_cast<diff>(indices[0]);
        auto read  = vector.begin() + static_cast<diff>(indices[0]);

        for (auto idx : indices) {
            auto end = vector.begin() + static_cast<diff>(idx);
            std::move(read, end, write);
            write += end - read;
            read  += end - read + 1;
        }

        std::move(read, vector.end(), write);
        write += vector.end() - read;

        vector.erase(write, vector.end());
    }
}
