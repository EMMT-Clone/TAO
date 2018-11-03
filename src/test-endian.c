#include <stdint.h>
#include <stdio.h>

static const union {
    uint8_t bytes[4];
    uint32_t bom;
} endian = {{1, 2, 3, 4}};

#define ENDIAN_BOM    (endian.bom)
#define LITTLE_ENDIAN 0x04030201
#define BIG_ENDIAN    0x01020304

int
main(int argc, char* argv[])
{
    const char* descr;
    if (ENDIAN_BOM == LITTLE_ENDIAN) {
        descr = "little endian";
    } else if (ENDIAN_BOM == BIG_ENDIAN) {
        descr = "big endian";
    } else {
        descr = "unknown";
    }
    printf("ENDIAN_BOM = 0x%08lx (%s byte order)\n",
           (unsigned long)ENDIAN_BOM, descr);
    return 0;
}
