#pragma once

#include <iostream>
#include <fstream>

//---------------------------------------------------------------------------

std::ostream& YTrace();

namespace YSystem
{
    void Init(int tracing_level);

    void TracingOn();
    
    void TracingOff();
}

//---------------------------------------------------------------------------


