#include <random>

#include <compute/device.h>
#include <compute/dsl.h>

using namespace luisa;
using namespace luisa::compute;
using namespace luisa::compute::dsl;

int main(int argc, char *argv[]) {
    
    Context context{argc, argv};
    auto device = Device::create(&context, "cuda");
    
    constexpr auto buffer_size = 1024u;
    
    auto buffer_a = device->allocate_buffer<float>(buffer_size);
    auto buffer_b = device->allocate_buffer<float>(buffer_size);
    
    std::vector<float> input(buffer_size);
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> distribution{0.0f, 1.0f};
    for (auto &&f : input) { f = distribution(rng); }
    
    auto input_copy = input;
    std::shuffle(input_copy.begin(), input_copy.end(), rng);
    
    auto scale_value = [&](Float k, ExprRef<float> x) -> Float {
        return k * x;
    };
    
    auto scale = 1.0f;
    auto kernel = device->compile_kernel("simple_test", [&] {
        
        Arg<const float *> a{buffer_a};
        Arg<float *> b{buffer_b};
        Arg<float> k{&scale};
        
        Auto tid = thread_id();
        If(tid < buffer_size) {
            b[tid] = scale_value(k, a[tid]);
        };
    });
    
    auto launch_index = 0u;
    
    for (auto i = 0; i < 20; i++) {
        device->launch([&](Dispatcher &dispatch) {
            scale = 3.0f;
            dispatch(buffer_a.copy_from(input_copy.data()));
            dispatch(kernel->parallelize(buffer_size), [&] { LUISA_INFO("Kernel #", launch_index++, " finished."); });
        });
    }
    
    std::vector<float> output(buffer_size);
    device->launch([&](Dispatcher &d) {
        scale = 2.0f;
        d(buffer_a.copy_from(input.data()));
        d(kernel->parallelize(buffer_size), [&] { LUISA_INFO("Kernel #", launch_index++, " finished."); });
        d(buffer_b.copy_to(output.data()));
    });
    
    for (auto i = 0; i < 20; i++) {
        device->launch([&](Dispatcher &d) {
            scale = 3.0f;
            d(buffer_a.copy_from(input_copy.data()));
            d(kernel->parallelize(buffer_size), [&] { LUISA_INFO("Kernel #", launch_index++, " finished."); });
        });
    }
    
    device->synchronize();
    
    LUISA_INFO("Done.");
    for (auto i = 0; i < buffer_size; i++) {
        if (output[i] != input[i] * 2.0f) {
            LUISA_WARNING("Fuck! Expected ", input[i] * 2.0f, ", got ", output[i]);
        }
    }
    LUISA_INFO("Results checked.");
}
