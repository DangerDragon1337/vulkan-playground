#include <iostream>
#include <stdexcept>

#include "MakeTriangle.hpp"

int main()
{
    try
    {
        MakeTriangle app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}