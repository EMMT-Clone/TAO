
#include "coaxpress.h"
#include "mikrotron-mc408x.h"

cxp_get(cam, WIDTH_ADDRESS, &width)

cxp_get(cam, DEVICE_SERIAL_NUMBER, ptr)
_cxp_size(TEST_PACKET_COUNT_TX)
cxp_get(cam, CONNECTION_CONFIG_DEFAULT, ptr)
cxp_set(cam, DEVICE_SERIAL_NUMBER, ptr)
