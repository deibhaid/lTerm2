#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iterm_core_version {
    int major;
    int minor;
    int patch;
} iterm_core_version;

const iterm_core_version *iterm_core_get_version(void);

#ifdef __cplusplus
}
#endif

