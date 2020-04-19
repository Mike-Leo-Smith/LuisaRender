//
// Created by Mike Smith on 2020/2/21.
//

#pragma once

#include "data_types.h"
#include "mathematics.h"
#include "interaction.h"
#include "light.h"

namespace luisa::illumination {

class Info {

private:
    uint8_t _tag{};
    uint8_t _index_hi{};
    uint16_t _index_lo{};

public:
    constexpr Info(uint tag, uint index) noexcept
        : _tag{static_cast<uint8_t>(tag)},
          _index_hi{static_cast<uint8_t>(index >> 16u)},
          _index_lo{static_cast<uint16_t>(index)} {}

public:
    [[nodiscard]] LUISA_DEVICE_CALLABLE constexpr auto tag() const noexcept { return static_cast<uint>(_tag); }
    [[nodiscard]] LUISA_DEVICE_CALLABLE constexpr auto index() const noexcept { return (static_cast<uint>(_index_hi) << 16u) | static_cast<uint>(_index_lo); }
};

struct SelectLightsKernelUniforms {
    uint light_count;
    uint max_queue_size;
};

LUISA_DEVICE_CALLABLE inline void uniform_select_lights(
    LUISA_DEVICE_SPACE const float *sample_buffer,
    LUISA_DEVICE_SPACE const Info *info_buffer,
    LUISA_DEVICE_SPACE Atomic<uint> *queue_sizes,
    LUISA_DEVICE_SPACE light::Selection *queues,
    uint its_count,
    SelectLightsKernelUniforms uniforms,
    uint tid) {
    
    if (tid < its_count) {
        auto light_info = info_buffer[min(static_cast<uint>(sample_buffer[tid] * uniforms.light_count), uniforms.light_count - 1u)];
        auto queue_index = luisa_atomic_fetch_add(queue_sizes[light_info.tag()], 1u);
        queues[light_info.tag() * uniforms.max_queue_size + queue_index] = {light_info.index(), tid};
    }
}

struct CollectLightInteractionsKernelUniforms {
    uint max_queue_size;
    uint sky_tag;
    bool has_sky;
};

LUISA_DEVICE_CALLABLE inline void collect_light_interactions(
    LUISA_DEVICE_SPACE const uint *its_instance_id_buffer,
    LUISA_DEVICE_SPACE const uint8_t *its_state_buffer,
    LUISA_DEVICE_SPACE const Info *instance_to_info_buffer,
    LUISA_DEVICE_SPACE Atomic<uint> *queue_sizes,
    LUISA_DEVICE_SPACE light::Selection *queues,
    uint its_count,
    CollectLightInteractionsKernelUniforms uniforms,
    uint tid) {
    
    if (tid < its_count) {
        auto state = its_state_buffer[tid];
        if (state & interaction::state::EMISSIVE) {  // hit on lights
            auto light_info = instance_to_info_buffer[its_instance_id_buffer[tid]];
            auto queue_index = luisa_atomic_fetch_add(queue_sizes[light_info.tag()], 1u);
            queues[light_info.tag() * uniforms.max_queue_size + queue_index] = {light_info.index(), tid};
        } else if (!(state & interaction::state::HIT) && uniforms.has_sky) {  // background
            auto queue_index = luisa_atomic_fetch_add(queue_sizes[uniforms.sky_tag], 1u);
            queues[uniforms.sky_tag * uniforms.max_queue_size + queue_index] = {0u, tid};
        }
    }
}

}

#ifndef LUISA_DEVICE_COMPATIBLE

#include <vector>

#include "geometry.h"
#include "kernel.h"
#include "viewport.h"

namespace luisa {

class Illumination : Noncopyable {

private:
    Device *_device;
    Geometry *_geometry;
    std::vector<std::shared_ptr<Light>> _lights;
    size_t _abstract_light_count;  // area_light_count = _lights.size() - _abstract_light_count
    bool _has_sky;
    uint _sky_tag;
    
    std::unique_ptr<Buffer<illumination::Info>> _info_buffer;
    std::vector<std::unique_ptr<TypelessBuffer>> _light_data_buffers;
    
    // for light sampling
    std::vector<uint> _light_sampling_dimensions;
    std::vector<std::unique_ptr<Kernel>> _light_sampling_kernels;
    std::vector<Light::SampleLightsDispatch> _light_sampling_dispatches;
    
    // for light evaluation
    std::vector<std::unique_ptr<Kernel>> _light_evaluation_kernels;
    std::vector<Light::EvaluateLightsDispatch> _light_evaluation_dispatches;
    
    // CDFs for light sampling
    std::unique_ptr<Buffer<float>> _cdf_buffer;
    std::unique_ptr<Buffer<illumination::Info>> _instance_to_light_info_buffer;
    
    // kernels
    std::unique_ptr<Kernel> _uniform_select_lights_kernel;
    std::unique_ptr<Kernel> _collect_light_interactions_kernel;

public:
    Illumination(Device *device, const std::vector<std::shared_ptr<Light>> &lights, Geometry *geometry);
    [[nodiscard]] uint tag_count() const noexcept { return static_cast<uint>(_light_data_buffers.size()); }
    
    void uniform_select_lights(KernelDispatcher &dispatch,
                               uint dispatch_extent,
                               BufferView<uint> ray_queue,
                               BufferView<uint> ray_queue_size,
                               Sampler &sampler,
                               BufferView<light::Selection> queues,
                               BufferView<uint> queue_sizes);
    
    void sample_lights(KernelDispatcher &dispatch,
                       uint dispatch_extent,
                       Sampler &sampler,
                       BufferView<uint> ray_indices,
                       BufferView<uint> ray_count,
                       BufferView<light::Selection> queues,
                       BufferView<uint> queue_sizes,
                       InteractionBufferSet &interactions,
                       LightSampleBufferSet &light_samples);
    
    void evaluate_light_emissions(KernelDispatcher &dispatch,
                                  uint dispatch_extent,
                                  BufferView<uint> ray_queue_size,
                                  BufferView<light::Selection> queues,
                                  BufferView<uint> queue_sizes,
                                  InteractionBufferSet &interactions);
    
};

}

#endif
