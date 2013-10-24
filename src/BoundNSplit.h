#pragma once

#include "common.h"

namespace Reyes
{

    struct PatchRange
    {
        Bound range;
        size_t depth;
        size_t patch_id;
    };

}
