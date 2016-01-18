#ifndef BRCM_MARSHALL_H
#define BRCM_MARSHALL_H

#include <stdint.h>
#include <sys/types.h>


enum brcm_marshall_types {
    BRCM_MARSHALL_ARG_UNDEF,
    BRCM_MARSHALL_ARG_UINT8,
    BRCM_MARSHALL_ARG_INT8,
    BRCM_MARSHALL_ARG_UINT16,
    BRCM_MARSHALL_ARG_INT16,
    BRCM_MARSHALL_ARG_UINT32,
    BRCM_MARSHALL_ARG_INT32,
    BRCM_MARSHALL_ARG_FLOAT,
    BRCM_MARSHALL_ARG_FUNC
};

#ifdef __cplusplus
extern "C" {
#endif
enum brcm_marshall_types brcm_marshall_get_type(uint8_t *rawdata, size_t n);
uint32_t brcm_marshall_get_value(uint8_t *rawdata, size_t n);
ssize_t brcm_marshall_init(uint8_t *rawdata, size_t n,
        enum brcm_marshall_types type,
        uint32_t value, uint8_t *payload, size_t psz);
ssize_t brcm_marshall_get_len(uint8_t *rawdata, size_t n);
ssize_t brcm_marshall_get_payload_len(uint8_t *rawdata, size_t n);
ssize_t brcm_marshall_get_payload(uint8_t *rawdata, size_t n, uint8_t *payload, size_t psz);


ssize_t brcm_marshall_func_init(uint8_t *rawdata, size_t n, int func_id);
ssize_t brcm_marshall_func_add_arg(uint8_t *rawdata, size_t n,
        enum brcm_marshall_types argt, uint32_t value, uint8_t *payload, size_t psz);
uint32_t brcm_marshall_func_id(uint8_t *rawdata, size_t n);
uint8_t *brcm_marshall_func_next_arg(uint8_t *rawdata, size_t n, uint8_t *arg_index, size_t *arg_len);
uint8_t *brcm_marshall_func_first_arg(uint8_t *rawdata, size_t n, size_t *arg_len);
#ifdef __cplusplus
}
#endif

#endif
