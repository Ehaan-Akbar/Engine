#include <iostream>
#include <stdexcept>

#include "App.h"

#include <glm/glm.hpp>
#include <vector>

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>

int main() {
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(175);

    {
        App app;

        try {
            app.run();
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

    }
    
    

    _CrtDumpMemoryLeaks();

    return EXIT_SUCCESS;
}