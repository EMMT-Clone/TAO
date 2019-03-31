/*
 * coaxpress.h -
 *
 * Constants for CoaXPress cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2017-2019, Éric Thiébaut.
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 */

#ifndef _COAXPRESS_H
#define _COAXPRESS_H 1

/*
 * Possible kind of registers (member __kind):
 *
 *  - Address: the register stores the address of the register storing the
 *    value;
 *  - Value: the register stores a value;
 *
 * Possible types (member __type):
 *
 *  - Address
 *  - Boolean
 *  - Command
 *  - Enumeration
 *  - Integer
 *  - Float
 *  - String
 *
 * Possible access modes (member __mode):
 *
 *  - ReadOnly
 *  - ReadWrite
 *  - WriteOnly
 *
 */
#undef Address
#undef Value
#undef Address
#undef Boolean
#undef Command
#undef Enumeration
#undef Integer
#undef Float
#undef String
#undef ReadOnly
#undef ReadWrite
#undef WriteOnly

/*
 * Bootstrap CoaXPress registers common to all CoaXPress compliant devices.
 */
#define CXP_STANDARD__kind                     Value
#define CXP_STANDARD__type                     Integer
#define CXP_STANDARD__size                     4
#define CXP_STANDARD__mode                     ReadOnly
#define CXP_STANDARD__addr                     0x00000000

#define CXP_REVISION__kind                     Value
#define CXP_REVISION__type                     Integer
#define CXP_REVISION__size                     4
#define CXP_REVISION__mode                     ReadOnly
#define CXP_REVISION__addr                     0x00000004

#define CXP_XML_MANIFEST_SIZE__kind            Value
#define CXP_XML_MANIFEST_SIZE__type            Integer
#define CXP_XML_MANIFEST_SIZE__size            4
#define CXP_XML_MANIFEST_SIZE__mode            ReadOnly
#define CXP_XML_MANIFEST_SIZE__addr            0x00000008

#define CXP_XML_MANIFEST_SELECTOR__kind        Value
#define CXP_XML_MANIFEST_SELECTOR__type        Integer
#define CXP_XML_MANIFEST_SELECTOR__size        4
#define CXP_XML_MANIFEST_SELECTOR__mode        ReadWrite
#define CXP_XML_MANIFEST_SELECTOR__addr        0x0000000C

#define CXP_XML_VERSION__kind                  Value
#define CXP_XML_VERSION__type                  Integer
#define CXP_XML_VERSION__size                  4
#define CXP_XML_VERSION__mode                  ReadOnly
#define CXP_XML_VERSION__addr                  0x00000010

#define CXP_XML_SCHEME_VERSION__kind           Value
#define CXP_XML_SCHEME_VERSION__type           Integer
#define CXP_XML_SCHEME_VERSION__size           4
#define CXP_XML_SCHEME_VERSION__mode           ReadOnly
#define CXP_XML_SCHEME_VERSION__addr           0x00000014

#define CXP_XML_URL_ADDRESS__kind              Address
#define CXP_XML_URL_ADDRESS__type              Integer
#define CXP_XML_URL_ADDRESS__size              4
#define CXP_XML_URL_ADDRESS__mode              ReadOnly
#define CXP_XML_URL_ADDRESS__addr              0x00000018

#define CXP_IIDC2_ADDRESS__kind                Address
#define CXP_IIDC2_ADDRESS__type                Integer
#define CXP_IIDC2_ADDRESS__size                4
#define CXP_IIDC2_ADDRESS__mode                ReadOnly
#define CXP_IIDC2_ADDRESS__addr                0x0000001C

#define CXP_DEVICE_VENDOR_NAME__kind           Value
#define CXP_DEVICE_VENDOR_NAME__type           String
#define CXP_DEVICE_VENDOR_NAME__size           32
#define CXP_DEVICE_VENDOR_NAME__mode           ReadOnly
#define CXP_DEVICE_VENDOR_NAME__addr           0x00002000

#define CXP_DEVICE_MODEL_NAME__kind            Value
#define CXP_DEVICE_MODEL_NAME__type            String
#define CXP_DEVICE_MODEL_NAME__size            32
#define CXP_DEVICE_MODEL_NAME__mode            ReadOnly
#define CXP_DEVICE_MODEL_NAME__addr            0x00002020

#define CXP_DEVICE_MANUFACTURER_INFO__kind     Value
#define CXP_DEVICE_MANUFACTURER_INFO__type     String
#define CXP_DEVICE_MANUFACTURER_INFO__size     48
#define CXP_DEVICE_MANUFACTURER_INFO__mode     ReadOnly
#define CXP_DEVICE_MANUFACTURER_INFO__addr     0x00002040

#define CXP_DEVICE_VERSION__kind               Value
#define CXP_DEVICE_VERSION__type               String
#define CXP_DEVICE_VERSION__size               32
#define CXP_DEVICE_VERSION__mode               ReadOnly
#define CXP_DEVICE_VERSION__addr               0x00002070

#define CXP_DEVICE_SERIAL_NUMBER__kind         Value
#define CXP_DEVICE_SERIAL_NUMBER__type         String
#define CXP_DEVICE_SERIAL_NUMBER__size         16
#define CXP_DEVICE_SERIAL_NUMBER__mode         ReadOnly
#define CXP_DEVICE_SERIAL_NUMBER__addr         0x000020B0

#define CXP_DEVICE_USER_ID__kind               Value
#define CXP_DEVICE_USER_ID__type               String
#define CXP_DEVICE_USER_ID__size               16
#define CXP_DEVICE_USER_ID__mode               ReadWrite
#define CXP_DEVICE_USER_ID__addr               0x000020C0

#define CXP_WIDTH_ADDRESS__kind                Address
#define CXP_WIDTH_ADDRESS__type                Integer
#define CXP_WIDTH_ADDRESS__size                4
#define CXP_WIDTH_ADDRESS__mode                ReadWrite
#define CXP_WIDTH_ADDRESS__addr                0x00003000

#define CXP_HEIGHT_ADDRESS__kind               Address
#define CXP_HEIGHT_ADDRESS__type               Integer
#define CXP_HEIGHT_ADDRESS__size               4
#define CXP_HEIGHT_ADDRESS__mode               ReadWrite
#define CXP_HEIGHT_ADDRESS__addr               0x00003004

#define CXP_ACQUISITION_MODE_ADDRESS__kind     Address
#define CXP_ACQUISITION_MODE_ADDRESS__type     Integer
#define CXP_ACQUISITION_MODE_ADDRESS__size     4
#define CXP_ACQUISITION_MODE_ADDRESS__mode     ReadWrite
#define CXP_ACQUISITION_MODE_ADDRESS__addr     0x00003008

#define CXP_ACQUISITION_START_ADDRESS__kind    Address
#define CXP_ACQUISITION_START_ADDRESS__type    Integer
#define CXP_ACQUISITION_START_ADDRESS__size    4
#define CXP_ACQUISITION_START_ADDRESS__mode    WriteOnly
#define CXP_ACQUISITION_START_ADDRESS__addr    0x0000300C

#define CXP_ACQUISITION_STOP_ADDRESS__kind     Address
#define CXP_ACQUISITION_STOP_ADDRESS__type     Integer
#define CXP_ACQUISITION_STOP_ADDRESS__size     4
#define CXP_ACQUISITION_STOP_ADDRESS__mode     WriteOnly
#define CXP_ACQUISITION_STOP_ADDRESS__addr     0x00003010

#define CXP_PIXEL_FORMAT_ADDRESS__kind         Address
#define CXP_PIXEL_FORMAT_ADDRESS__type         Integer
#define CXP_PIXEL_FORMAT_ADDRESS__size         4
#define CXP_PIXEL_FORMAT_ADDRESS__mode         ReadWrite
#define CXP_PIXEL_FORMAT_ADDRESS__addr         0x00003014

#define CXP_DEVICE_TAP_GEOMETRY_ADDRESS__kind  Address
#define CXP_DEVICE_TAP_GEOMETRY_ADDRESS__type  Integer
#define CXP_DEVICE_TAP_GEOMETRY_ADDRESS__size  4
#define CXP_DEVICE_TAP_GEOMETRY_ADDRESS__mode  ReadWrite
#define CXP_DEVICE_TAP_GEOMETRY_ADDRESS__addr  0x00003018

#define CXP_IMAGE1_STREAM_ID_ADDRESS__kind     Address
#define CXP_IMAGE1_STREAM_ID_ADDRESS__type     Integer
#define CXP_IMAGE1_STREAM_ID_ADDRESS__size     4
#define CXP_IMAGE1_STREAM_ID_ADDRESS__mode     ReadWrite
#define CXP_IMAGE1_STREAM_ID_ADDRESS__addr     0x0000301C

#define CXP_CONNECTION_RESET__kind             Value
#define CXP_CONNECTION_RESET__type             Command
#define CXP_CONNECTION_RESET__size             4
#define CXP_CONNECTION_RESET__mode             ReadWrite
#define CXP_CONNECTION_RESET__addr             0x00004000
#define CXP_CONNECTION_RESET__comm             1

#define CXP_DEVICE_CONNECTION_ID__kind         Value
#define CXP_DEVICE_CONNECTION_ID__type         Integer
#define CXP_DEVICE_CONNECTION_ID__size         4
#define CXP_DEVICE_CONNECTION_ID__mode         ReadOnly
#define CXP_DEVICE_CONNECTION_ID__addr         0x00004004

#define CXP_MASTER_HOST_CONNECTION_ID__kind    Value
#define CXP_MASTER_HOST_CONNECTION_ID__type    Integer
#define CXP_MASTER_HOST_CONNECTION_ID__size    4
#define CXP_MASTER_HOST_CONNECTION_ID__mode    ReadWrite
#define CXP_MASTER_HOST_CONNECTION_ID__addr    0x00004008

#define CXP_CONTROL_PACKET_SIZE_MAX__kind      Value
#define CXP_CONTROL_PACKET_SIZE_MAX__type      Integer
#define CXP_CONTROL_PACKET_SIZE_MAX__size      4
#define CXP_CONTROL_PACKET_SIZE_MAX__mode      ReadOnly
#define CXP_CONTROL_PACKET_SIZE_MAX__addr      0x0000400C

#define CXP_STREAM_PACKET_SIZE_MAX__kind       Value
#define CXP_STREAM_PACKET_SIZE_MAX__type       Integer
#define CXP_STREAM_PACKET_SIZE_MAX__size       4
#define CXP_STREAM_PACKET_SIZE_MAX__mode       ReadWrite
#define CXP_STREAM_PACKET_SIZE_MAX__addr       0x00004010

#define CXP_CONNECTION_CONFIG__kind            Value
#define CXP_CONNECTION_CONFIG__type            Enumeration
#define CXP_CONNECTION_CONFIG__size            4
#define CXP_CONNECTION_CONFIG__mode            ReadWrite
#define CXP_CONNECTION_CONFIG__addr            0x00004014

#define CXP_CONNECTION_CONFIG_DEFAULT__kind    Value
#define CXP_CONNECTION_CONFIG_DEFAULT__type    Integer
#define CXP_CONNECTION_CONFIG_DEFAULT__size    4
#define CXP_CONNECTION_CONFIG_DEFAULT__mode    ReadOnly
#define CXP_CONNECTION_CONFIG_DEFAULT__addr    0x00004018

#define CXP_TEST_MODE__kind                    Value
#define CXP_TEST_MODE__type                    Integer
#define CXP_TEST_MODE__size                    4
#define CXP_TEST_MODE__mode                    ReadWrite
#define CXP_TEST_MODE__addr                    0x0000401C

#define CXP_TEST_ERROR_COUNT_SELECTOR__kind    Value
#define CXP_TEST_ERROR_COUNT_SELECTOR__type    Integer
#define CXP_TEST_ERROR_COUNT_SELECTOR__size    4
#define CXP_TEST_ERROR_COUNT_SELECTOR__mode    ReadWrite
#define CXP_TEST_ERROR_COUNT_SELECTOR__addr    0x00004020

#define CXP_TEST_ERROR_COUNT__kind             Value
#define CXP_TEST_ERROR_COUNT__type             Integer
#define CXP_TEST_ERROR_COUNT__size             4
#define CXP_TEST_ERROR_COUNT__mode             ReadWrite
#define CXP_TEST_ERROR_COUNT__addr             0x00004024

#define CXP_TEST_PACKET_COUNT_TX__kind         Value
#define CXP_TEST_PACKET_COUNT_TX__type         uint64
#define CXP_TEST_PACKET_COUNT_TX__size         8
#define CXP_TEST_PACKET_COUNT_TX__mode         ReadWrite
#define CXP_TEST_PACKET_COUNT_TX__addr         0x00004028

#define CXP_TEST_PACKET_COUNT_RX__kind         Value
#define CXP_TEST_PACKET_COUNT_RX__type         uint64
#define CXP_TEST_PACKET_COUNT_RX__size         8
#define CXP_TEST_PACKET_COUNT_RX__mode         ReadWrite
#define CXP_TEST_PACKET_COUNT_RX__addr         0x00004030

#define CXP_HS_UP_CONNECTION__kind             Value
#define CXP_HS_UP_CONNECTION__type             Integer
#define CXP_HS_UP_CONNECTION__size             4
#define CXP_HS_UP_CONNECTION__mode             ReadOnly
#define CXP_HS_UP_CONNECTION__addr             0x0000403C

/* Start of manufacturer specific register space. */
#define CXP_MANUFACTURER__addr                 0x00006000

/*
 * Bits for CONNECTION_CONFIG register.  The value is a combination of speed
 * and number of connections (not all combinations are possible, see camera
 * manual).
 */
#define CXP_CONNECTION_CONFIG_SPEED_1250      0x00028
#define CXP_CONNECTION_CONFIG_SPEED_2500      0x00030
#define CXP_CONNECTION_CONFIG_SPEED_3125      0x00038
#define CXP_CONNECTION_CONFIG_SPEED_5000      0x00040
#define CXP_CONNECTION_CONFIG_SPEED_6250      0x00048
#define CXP_CONNECTION_CONFIG_CONNECTION_1    0x10000
#define CXP_CONNECTION_CONFIG_CONNECTION_2    0x20000
#define CXP_CONNECTION_CONFIG_CONNECTION_3    0x30000
#define CXP_CONNECTION_CONFIG_CONNECTION_4    0x40000


#define _cxp_verbatim(a)  a
#define _cxp_stringify(a)  #a

/* Join without expansion. */
#define _cxp_join(a1,a2)        a1##a2
#define _cxp_join2(a1,a2)       a1##a2
#define _cxp_join3(a1,a2,a3)    a1##a2##a3
#define _cxp_join4(a1,a2,a3,a4) a1##a2##a3##a4

/* Join with expansion. */
#define _cxp_xjoin(a1,a2)        _cxp_join(a1,a2)
#define _cxp_xjoin2(a1,a2)       _cxp_join2(a1,a2)
#define _cxp_xjoin3(a1,a2,a3)    _cxp_join3(a1,a2,a3)
#define _cxp_xjoin4(a1,a2,a3,a4) _cxp_join4(a1,a2,a3,a4)

#define _cxp_kind(id) CXP_##id##__kind
#define _cxp_type(id) CXP_##id##__type
#define _cxp_size(id) CXP_##id##__size
#define _cxp_addr(id) CXP_##id##__addr
#define _cxp_mode(id) CXP_##id##__mode
#define _cxp_comm(id) CXP_##id##__comm
#define _cxp_min(id)  CXP_##id##__min
#define _cxp_max(id)  CXP_##id##__max
#define _cxp_inc(id)  CXP_##id##__inc

#define cxp_kind(id) _cxp_kind(id)
#define cxp_type(id) _cxp_type(id)
#define cxp_size(id) _cxp_size(id)
#define cxp_addr(id) _cxp_addr(id)
#define cxp_mode(id) _cxp_mode(id)
#define cxp_comm(id) _cxp_comm(id)
#define cxp_min(id)  _cxp_min(id)
#define cxp_max(id)  _cxp_max(id)
#define cxp_inc(id)  _cxp_inc(id)

/**
 * Set a CoaXPress register.
 */
#define cxp_get(cam, id, ptr) \
    _cxp_xjoin(_cxp_get_,_cxp_mode(id))(cam, id, ptr)

/**
 * Execute a CoaXPress command.
 */
#define cxp_exec(cam, id) cxp_set(cam, id, cxp_comm(id))

/**
 * Set a CoaXPress register.
 */
#define cxp_set(cam, id, val) \
    _cxp_xjoin(_cxp_set_,_cxp_mode(id))(cam, id, val)


/* Cascading macros for getting a CoaXPress parameter.  We first manage to
   raise a compilation error is parameter is not readable; we otherwise
   dispatch to the specific reader function. */

#define _cxp_get_ReadOnly(cam, id, ptr) \
    _cxp_xjoin3(_cxp_get_,_cxp_kind(id),_cxp_type(id))(cam, id, ptr)

#define _cxp_get_ReadWrite(cam, id, ptr) \
    _cxp_xjoin3(_cxp_get_,_cxp_kind(id),_cxp_type(id))(cam, id, ptr)

#define _cxp_get_WriteOnly(cam, id, ptr) \
    TRYING_TO_READ_WRITEONLY_PARAMETER_##id

#define _cxp_get_Unknown(cam, id, ptr) \
    TRYING_TO_READ_UNKNOWN_PARAMETER_##id

#define _cxp_get_ValueString(cam, id, ptr) \
    cxp_read_string(cam, _cxp_addr(id), _cxp_size(id), ptr)

#define _cxp_get_ValueCommand(cam, id, ptr) \
    _cxp_get_ValueInteger(cam, id, ptr)

#define _cxp_get_ValueEnumeration(cam, id, ptr) \
    _cxp_get_ValueInteger(cam, id, ptr)

#define _cxp_get_ValueInteger(cam, id, ptr) \
    _cxp_xjoin(_cxp_get_ValueInteger,_cxp_size(id))(cam, id, ptr)

#define _cxp_get_ValueFloat(cam, id, ptr) \
    _cxp_xjoin(_cxp_get_ValueFloat,_cxp_size(id))(cam, id, ptr)

#define _cxp_get_ValueInteger4(cam, id, ptr) \
    cxp_read_uint32(cam, _cxp_addr(id), ptr)

#define _cxp_get_ValueInteger8(cam, id, ptr) \
    cxp_read_uint64(cam, _cxp_addr(id), ptr)

#define _cxp_get_ValueFloat4(cam, id, ptr) \
    cxp_read_float32(cam, _cxp_addr(id), ptr)

#define _cxp_get_ValueFloat8(cam, id, ptr) \
    cxp_read_float64(cam, _cxp_addr(id), ptr)

#define _cxp_get_AddressInteger(cam, id, ptr) \
    _cxp_xjoin(_cxp_get_AddressInteger,_cxp_size(id))(cam, id, ptr)

#define _cxp_get_AddressFloat(cam, id, ptr) \
    _cxp_xjoin(_cxp_get_AddressFloat,_cxp_size(id))(cam, id, ptr)

#define _cxp_get_AddressInteger4(cam, id, ptr)    \
    cxp_read_indirect_uint32(cam, _cxp_addr(id), ptr)

#define _cxp_get_AddressInteger8(cam, id, ptr)    \
    cxp_read_indirect_uint64(cam, _cxp_addr(id), ptr)


/* Cascading macros for setting a CoaXPress parameter.  We first manage to
   raise a compilation error is parameter is not writable; we otherwise
   dispatch to the specific writer function. */

#define _cxp_set_ReadOnly(cam, id, val) \
    TRYING_TO_WRITE_READONLY_PARAMETER_##id

#define _cxp_set_ReadWrite(cam, id, val) \
    _cxp_xjoin3(_cxp_set_,_cxp_kind(id),_cxp_type(id))(cam, id, val)

#define _cxp_set_WriteOnly(cam, id, val) \
    _cxp_xjoin3(_cxp_set_,_cxp_kind(id),_cxp_type(id))(cam, id, val)

#define _cxp_set_Unknown(cam, id, val) \
    TRYING_TO_WRITE_UNKNOWN_PARAMETER_##id

#define _cxp_set_ValueString(cam, id, val) \
    cxp_write_string(cam, _cxp_addr(id), _cxp_size(id), val)

#define _cxp_set_ValueCommand(cam, id, val) \
    _cxp_set_ValueInteger(cam, id, val)

#define _cxp_set_ValueEnumeration(cam, id, val) \
    _cxp_set_ValueInteger(cam, id, val)

#define _cxp_set_ValueInteger(cam, id, val) \
    _cxp_xjoin(_cxp_set_ValueInteger,_cxp_size(id))(cam, id, val)

#define _cxp_set_ValueFloat(cam, id, val) \
    _cxp_xjoin(_cxp_set_ValueFloat,_cxp_size(id))(cam, id, val)

#define _cxp_set_ValueInteger4(cam, id, val) \
    cxp_write_uint32(cam, _cxp_addr(id), val)

#define _cxp_set_ValueInteger8(cam, id, val) \
    cxp_write_uint64(cam, _cxp_addr(id), val)

#define _cxp_set_ValueFloat4(cam, id, val) \
    cxp_write_float32(cam, _cxp_addr(id), val)

#define _cxp_set_ValueFloat8(cam, id, val) \
    cxp_write_float64(cam, _cxp_addr(id), val)

/*
 * Length of textual CoaXPress parameters.
 */
#define CXP_DEVICE_VENDOR_NAME_LENGTH         32
#define CXP_DEVICE_MODEL_NAME_LENGTH          32
#define CXP_DEVICE_MANUFACTURER_INFO_LENGTH   48
#define CXP_DEVICE_VERSION_LENGTH             32
#define CXP_DEVICE_SERIAL_NUMBER_LENGTH       16
#define CXP_DEVICE_USER_ID_LENGTH             16

/*
 * Value returned by reading at CXP_STANDARD register.
 */
#define CXP_MAGIC 0xC0A79AE5

#endif /* _COAXPRESS_H */
