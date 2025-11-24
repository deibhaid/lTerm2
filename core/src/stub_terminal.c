#include "iterm_core.h"

static const iterm_core_version version = {
    .major = 0,
    .minor = 0,
    .patch = 1,
};

const iterm_core_version *
iterm_core_get_version(void)
{
    return &version;
}

