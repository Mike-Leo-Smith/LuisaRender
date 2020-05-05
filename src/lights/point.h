//
// Created by Mike Smith on 2020/2/17.
//

#pragma once

#include <core/ray.h>
#include <core/interaction.h>
#include <core/mathematics.h>
#include <core/light.h>

namespace luisa::light::point {

struct Data {
    float3 position;
    float3 emission;
};

LUISA_DEVICE_CALLABLE inline void generate_samples(
    LUISA_DEVICE_SPACE const Data *data_buffer,
    LUISA_DEVICE_SPACE const light::Selection *queue,
    uint queue_size,
    LUISA_DEVICE_SPACE uint8_t *its_state_buffer,
    LUISA_DEVICE_SPACE const float3 *its_position_buffer,
    LUISA_DEVICE_SPACE float4 *Li_and_pdf_w_buffer,
    LUISA_DEVICE_SPACE bool *is_delta_buffer,
    LUISA_DEVICE_SPACE Ray *shadow_ray_buffer,
    uint tid) {
    
    if (tid < queue_size) {
        auto selection = queue[tid];
        if (its_state_buffer[selection.interaction_index] & interaction::state::HIT) {
            auto light_data = data_buffer[selection.data_index];
            auto its_p = its_position_buffer[selection.interaction_index];
            auto d = light_data.position - its_p;
            auto dd = max(1e-6f, dot(d, d));
            Li_and_pdf_w_buffer[selection.interaction_index] = make_float4(light_data.emission * (1.0f / dd), 1.0f);
            is_delta_buffer[selection.interaction_index] = true;
            auto distance = sqrt(dd);
            auto wo = d * (1.0f / distance);
            shadow_ray_buffer[selection.interaction_index] = make_ray(its_p, wo, 1e-4f, distance);
            its_state_buffer[selection.interaction_index] |= interaction::state::DELTA_LIGHT;
        } else {
            shadow_ray_buffer[selection.interaction_index].max_distance = -1.0f;
        }
    }
}

}