#include "TestAPI.h"

#include <iostream>

namespace ScriptAPI
{
    void TestAPI::SayHello()
    {
        std::cout << "[C++/CLI Bridge] Hello from ScriptAPI!" << std::endl;
    }

}