//
// Created by Mike Smith on 2020/11/21.
//

#pragma once

#include <vector>
#include <array>
#include <span>
#include <type_traits>

namespace luisa {

class MemoryArena {

public:
    static constexpr auto block_size = 256ul * 1024ul;
    static constexpr auto max_alignment = 16ul;
    using Block = std::array<uint8_t, block_size>;

private:
    std::vector<std::unique_ptr<Block>> _blocks;
    size_t _offset{0u};

public:
    template<typename T, typename ...Args>
    [[nodiscard]] T *create(Args &&...args) {
        auto memory = allocate<T>(1u);
        return new(memory.data()) T(std::forward<Args>(args)...);
    }
    
    template<typename T = uint8_t, size_t alignment = std::max(std::alignment_of_v<T>, static_cast<size_t>(16u))>
    [[nodiscard]] std::span<T> allocate(size_t n) {
        
        constexpr auto size = sizeof(T);
        static_assert(size <= block_size && alignment <= max_alignment && std::is_trivially_destructible_v<T>);
        
        auto byte_size = n * size;
        if (byte_size > block_size) { throw std::invalid_argument{"too many elements"}; }
        auto aligned_offset = (_offset + alignment - 1u) / alignment * alignment;
        if (_blocks.empty() || aligned_offset >= block_size) {
            aligned_offset = 0u;
            _blocks.emplace_back(std::make_unique<Block>());
        }
        auto memory = _blocks.back()->data() + aligned_offset;
        _offset = aligned_offset + byte_size;
        return {reinterpret_cast<T *>(memory), n};
    }
    
    [[nodiscard]] std::string_view allocate_string(std::string_view sv) {
        auto buffer = allocate<char>(sv.size() + 1);
        std::strncpy(buffer.data(), sv.data(), sv.size());
        return {buffer.data(), sv.size()};
    }
    
    void reset() noexcept {
        _blocks.clear();
        _offset = 0u;
    }
};

}
