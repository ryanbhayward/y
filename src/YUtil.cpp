#include "YUtil.h"


std::string YUtil::HashString(uint32_t hash)
{
    std::ostringstream os;
    os << std::setfill('0');
    os << "0x" << std::setw(8) << std::hex << hash;
    return os.str();
}
