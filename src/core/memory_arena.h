//
// Created by Mike Smith on 2020/11/21.
//

#pragma once

#include <vector>
#include <array>
#include <type_traits>

namespace luisa {

class MemoryArena {

public:
    static constexpr auto block_size = 256ul * 1024ul;
    using Block = std::array<uint8_t, block_size>;

private:
    std::vector<std::unique_ptr<Block>> _blocks;
    size_t _offset{0u};

public:
    template<typename T, typename ...Args>
    T *create(Args &&...args) {
        static_assert(sizeof(T) <= block_size && std::is_trivially_destructible_v<T>);
        constexpr auto alignment = std::alignment_of_v<T>;
        _offset = (_offset + alignment - 1u) / alignment * alignment;
        if (_blocks.empty() || _offset >= block_size) {
            _offset = 0u;
            _blocks.emplace_back(std::make_unique<Block>());
        }
        auto memory = _blocks.back()->data() + _offset;
        _offset += sizeof(T);
        return new(memory) T(std::forward<Args>(args)...);
    }
};

}
