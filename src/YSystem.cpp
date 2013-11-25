#include "YSystem.h"

namespace {

    // Like in Fuego SgDebug: this stream is never opened.
    std::ofstream s_nullStream;

    std::ostream* s_traceStream;

}

std::ostream& YTrace()
{
    return *s_traceStream;
}

void YSystem::Init(int tracing_level)
{
    if (tracing_level == 0)
        TracingOff();
    else 
        TracingOn();
}

void YSystem::TracingOn()
{
    s_traceStream = &std::cerr;
}

void YSystem::TracingOff()
{
    s_traceStream = &s_nullStream;
}
