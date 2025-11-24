#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lterm_core_version {
    int major;
    int minor;
    int patch;
} lterm_core_version;

const lterm_core_version *lterm_core_get_version(void);

#ifdef __cplusplus
}
#endif

