#include "lterm_core.h"

static const lterm_core_version version = {
    .major = 0,
    .minor = 0,
    .patch = 1,
};

const lterm_core_version *
lterm_core_get_version(void)
{
    return &version;
}

