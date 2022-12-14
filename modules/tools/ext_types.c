#include "ext_funcs.h"
#include "ext_types.h"

__EXPORT_C_START

int put_octet(octet_t src) {
    PRINT("%c%c%c%c%c%c%c%c",
          (!!(src & 0x80)) + '0', (!!(src & 0x40)) + '0',
          (!!(src & 0x20)) + '0', (!!(src & 0x10)) + '0',
          (!!(src & 0x08)) + '0', (!!(src & 0x04)) + '0',
          (!!(src & 0x02)) + '0', (!!(src & 0x01)) + '0');
    return SUCCESS_CODE;
}

int put_octets(octet_t *src, size_t size, char sep) {
    if (UNLIKELY(size == 0))
        return SUCCESS_CODE;
    else {
        size_t i = 0;
        --size;
        for (; i < size; ++i) {
            put_octet(*(src + i));
            PRINT("%c", sep);
        }
        put_octet(*(src + size));
    }
    return SUCCESS_CODE;
}

int reverse_octets_bits(octet_t *dst, size_t size) {
    size_t i = 0;
    octet_t temp;
    for (; ((i + 1) << 1) <= size; ++i) {
        reverse_octet(dst + i);
        reverse_octet(dst + size - i - 1);
        temp = *(dst + i);
        *(dst + i) = *(dst + size - i - 1);
        *(dst + size - i - 1) = temp;
    }
    if (size & 1)
        reverse_octet(dst + (size >> 1));
    return SUCCESS_CODE;
}

int reverse_octets(octet_t *dst, size_t size) {
    size_t i = 0;
    octet_t temp;
    for (; ((i + 1) << 1) <= size; ++i) {
        temp = *(dst + i);
        *(dst + i) = *(dst + size - i - 1);
        *(dst + size - i - 1) = temp;
    }
    return SUCCESS_CODE;
}

__EXPORT_C_END

