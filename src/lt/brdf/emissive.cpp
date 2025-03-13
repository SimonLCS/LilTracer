#include "emissive.h"

namespace LT_NAMESPACE {


/////////////////////
// Emissive 
///////////////////
Spectrum Emissive::emission()
{
    return intensity.mean;
}

} // namespace LT_NAMESPACE