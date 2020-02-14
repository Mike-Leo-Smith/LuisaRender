//
// Created by Mike Smith on 2020/2/2.
//

#pragma once

#include <variant>
#include <string>
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <memory>
#include <iostream>

#include <util/noncopyable.h>
#include <util/exception.h>
#include "data_types.h"
#include "device.h"

namespace luisa {
class ParameterSet;
}

namespace luisa {

template<typename BaseClass>
using NodeCreator = std::function<std::unique_ptr<BaseClass>(Device *, const ParameterSet &)>;

template<typename BaseClass>
class NodeCreatorRegistry {

private:
    std::unordered_map<std::string_view, NodeCreator<BaseClass>> _creators;

public:
    NodeCreatorRegistry() noexcept = default;
    
    void emplace(std::string_view derived_name, NodeCreator<BaseClass> creator) noexcept {
        assert(_creators.find(derived_name) == _creators.end());  // FIXME: Failure on ThinLensCamera
        _creators.emplace(derived_name, std::move(creator));
    }
    
    NodeCreator<BaseClass> &operator[](std::string_view derived_name) noexcept {
        auto iter = _creators.find(derived_name);
        LUISA_ERROR_IF(iter == _creators.end(), "unregistered node creator for derived class: ", derived_name);
        return iter->second;
    }

#ifndef NDEBUG
    void debug() const noexcept {
        for (auto &&item : _creators) {
            std::cout << item.first << "\n";
        }
        std::cout << std::endl;
    }
#endif
    
};

#define LUISA_MAKE_BASE_NODE_CLASS_MATCHER(BaseClass)                                                                                                       \
    class BaseClass;                                                                                                                                        \
    namespace _impl { template<typename T, std::enable_if_t<std::is_base_of_v<BaseClass, T>, int> = 0> auto base_node_class_impl(T &&) -> BaseClass * {} }

LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Filter)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Film)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Camera)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Shape)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Transform)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Light)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Material)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Integrator)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Render)
LUISA_MAKE_BASE_NODE_CLASS_MATCHER(Sampler)

#undef LUISA_MAKE_BASE_NODE_CLASS_MATCHER

namespace _impl {

template<typename BaseClass>
void register_node_creator(std::string_view derived_name, NodeCreator<BaseClass> creator) noexcept {
    BaseClass::_creators.emplace(derived_name, std::move(creator));
}

template<typename T>
using BaseNodeClass = std::remove_pointer_t<decltype(base_node_class_impl(std::declval<T>()))>;

}

#define LUISA_MAKE_NODE_CREATOR_REGISTRY(BaseClass)                                                                               \
    friend class ParameterSet;                                                                                                    \
    friend void _impl::register_node_creator<BaseClass>(std::string_view derived_name, NodeCreator<BaseClass> creator) noexcept;  \
    inline static NodeCreatorRegistry<BaseClass> _creators

#define LUISA_REGISTER_NODE_CREATOR(derived_type_name, DerivedClass)                                                                    \
    namespace _impl {                                                                                                                   \
        static struct Register##DerivedClass##Helper {                                                                                  \
            Register##DerivedClass##Helper() noexcept {                                                                                 \
                register_node_creator<BaseNodeClass<DerivedClass>>(derived_type_name, [](Device *device, const ParameterSet &params) {  \
                    return std::make_unique<DerivedClass>(device, params);                                                              \
                });                                                                                                                     \
            }                                                                                                                           \
        } _registration_##DerivedClass{};                                                                                               \
    }

class Node : public Noncopyable {

public:
    friend class ParameterSet;

protected:
    Device *_device;

public:
    explicit Node(Device *device) noexcept : _device{device} {}
    virtual ~Node() noexcept = default;
    [[nodiscard]] Device &device() noexcept { return *_device; }
    
};

}
