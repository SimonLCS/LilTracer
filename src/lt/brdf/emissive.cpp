#include "emissive.h"

namespace LT_NAMESPACE {


/////////////////////
// Emissive 
///////////////////
Spectrum Emissive::emission()
{
    return intensity;
}

} // namespace LT_NAMESPACE