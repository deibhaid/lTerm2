#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ITERM_CSI_PARAM_MAX 16

typedef struct {
    int32_t cmd;  // packed prefix/intermediate/final bytes
    int count;
    int p[ITERM_CSI_PARAM_MAX];
    int sub_params[ITERM_CSI_PARAM_MAX][ITERM_CSI_PARAM_MAX];
} iterm_csi_param;

static inline void
iterm_csi_param_reset(iterm_csi_param *param)
{
    if (!param) {
        return;
    }
    param->cmd = 0;
    param->count = 0;
    for (int i = 0; i < ITERM_CSI_PARAM_MAX; ++i) {
        param->p[i] = -1;
        for (int j = 0; j < ITERM_CSI_PARAM_MAX; ++j) {
            param->sub_params[i][j] = -1;
        }
    }
}

#define ITERM_PACKED_CSI(prefix, intermediate, final) \
    (((prefix) << 16) | ((intermediate) << 8) | (final))

static inline int
iterm_csi_final_byte(int32_t packed)
{
    return packed & 0xff;
}

static inline int
iterm_csi_intermediate_byte(int32_t packed)
{
    return (packed >> 8) & 0xff;
}

static inline int
iterm_csi_prefix_byte(int32_t packed)
{
    return (packed >> 16) & 0xff;
}

#ifdef __cplusplus
}
#endif

