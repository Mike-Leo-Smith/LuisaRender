#include <core/render.h>

using namespace luisa;

int main(int argc, char *argv[]) {
    
    try {
        LUISA_EXCEPTION_IF(argc != 2, "No input file specific");
        Parser{Device::create("Metal").get()}.parse(argv[1])->execute();
    } catch (const std::exception &e) {
        LUISA_ERROR(e.what());
    }
}
