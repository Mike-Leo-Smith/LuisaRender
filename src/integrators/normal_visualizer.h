//
// Created by Mike Smith on 2020/2/19.
//

#pragma once

#include <core/data_types.h>

namespace luisa::integrator::normal {

LUISA_DEVICE_CALLABLE inline void prepare_for_frame(
    LUISA_DEVICE_SPACE uint *ray_queue,
    LUISA_DEVICE_SPACE uint &ray_queue_size,
    uint pixel_count,
    uint tid) noexcept {
    
    if (tid < pixel_count) {
        if (tid == 0u) { ray_queue_size = 0u; }
        ray_queue[tid] = tid;
    }

}

}

#ifndef LUISA_DEVICE_COMPATIBLE

#include <core/integrator.h>
#include <core/geometry.h>

namespace luisa {

class NormalVisualizer : public Integrator {

protected:
    std::unique_ptr<Buffer<uint>> _ray_queue_size;
    std::unique_ptr<Buffer<uint>> _ray_queue;
    std::unique_ptr<Buffer<Ray>> _ray_buffer;
    std::unique_ptr<Buffer<float2>> _ray_pixel_buffer;
    std::unique_ptr<Buffer<float3>> _ray_throughput_buffer;
    InteractionBufferSet _interaction_buffers;
    std::unique_ptr<Kernel> _prepare_for_frame_kernel;
    
    void _prepare_for_frame() override;

public:
    NormalVisualizer(Device *device, const ParameterSet &parameter_set[[maybe_unused]]);
    void render_frame(KernelDispatcher &dispatch) override;
    
};

LUISA_REGISTER_NODE_CREATOR("Normal", NormalVisualizer)

}

#endif