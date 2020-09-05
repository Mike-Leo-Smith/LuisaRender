//
// Created by Mike Smith on 2020/9/5.
//

#pragma once

#include "plugin.h"

namespace luisa::render {

class Transform {

public:
    [[nodiscard]] virtual float4x4 matrix(float time) const noexcept = 0;
};

}