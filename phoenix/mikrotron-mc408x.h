/*
 * mikrotron-mc408x.h -
 *
 * Constants for Mikrotron MC408x cameras.  These values have been
 * semi-automatically set from the XML configuration file of the camera.
 * See file "coaxpress.h" for explanations.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of TAO software (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2017-2018, Éric Thiébaut.
 * Copyright (C) 2016, Éric Thiébaut & Jonathan Léger.
 */

#ifndef _MIKROTRON_MC408X_H
#define _MIKROTRON_MC408X_H 1

#define CXP_ACQUISITION_MODE__kind    Value
#define CXP_ACQUISITION_MODE__type    Integer
#define CXP_ACQUISITION_MODE__size    4
#define CXP_ACQUISITION_MODE__mode    ReadWrite
#define CXP_ACQUISITION_MODE__addr    0x8200

#define CXP_ACQUISITION_START__kind    Value
#define CXP_ACQUISITION_START__type    Command
#define CXP_ACQUISITION_START__size    4
#define CXP_ACQUISITION_START__mode    WriteOnly
#define CXP_ACQUISITION_START__addr    0x8204
#define CXP_ACQUISITION_START__comm    1

#define CXP_ACQUISITION_STOP__kind    Value
#define CXP_ACQUISITION_STOP__type    Command
#define CXP_ACQUISITION_STOP__size    4
#define CXP_ACQUISITION_STOP__mode    WriteOnly
#define CXP_ACQUISITION_STOP__addr    0x8208
#define CXP_ACQUISITION_STOP__comm    1

#define CXP_ACQUISITION_BURST_FRAME_COUNT__kind    Value
#define CXP_ACQUISITION_BURST_FRAME_COUNT__type    Integer
#define CXP_ACQUISITION_BURST_FRAME_COUNT__size    4
#define CXP_ACQUISITION_BURST_FRAME_COUNT__mode    ReadWrite
#define CXP_ACQUISITION_BURST_FRAME_COUNT__addr    0x8914

#define CXP_TRIGGER_SELECTOR__kind    Value
#define CXP_TRIGGER_SELECTOR__type    Enumeration
#define CXP_TRIGGER_SELECTOR__size    4
#define CXP_TRIGGER_SELECTOR__mode    ReadWrite
#define CXP_TRIGGER_SELECTOR__addr    0x8900

#define CXP_TRIGGER_MODE__kind    Value
#define CXP_TRIGGER_MODE__type    Enumeration
#define CXP_TRIGGER_MODE__size    4
#define CXP_TRIGGER_MODE__mode    ReadWrite
#define CXP_TRIGGER_MODE__addr    0x8904

#define CXP_TRIGGER_SOURCE__kind    Value
#define CXP_TRIGGER_SOURCE__type    Enumeration
#define CXP_TRIGGER_SOURCE__size    4
#define CXP_TRIGGER_SOURCE__mode    ReadWrite
#define CXP_TRIGGER_SOURCE__addr    0x8908

#define CXP_TRIGGER_ACTIVATION__kind    Value
#define CXP_TRIGGER_ACTIVATION__type    Enumeration
#define CXP_TRIGGER_ACTIVATION__size    4
#define CXP_TRIGGER_ACTIVATION__mode    ReadWrite
#define CXP_TRIGGER_ACTIVATION__addr    0x890C

#define CXP_TRIGGER_COUNT__kind    Value
#define CXP_TRIGGER_COUNT__type    Unknown
#define CXP_TRIGGER_COUNT__size    Unknown
#define CXP_TRIGGER_COUNT__mode    Unknown
#define CXP_TRIGGER_COUNT__addr    0x891C

#define CXP_TRIGGER_DEBOUNCER__kind    Value
#define CXP_TRIGGER_DEBOUNCER__type    Unknown
#define CXP_TRIGGER_DEBOUNCER__size    Unknown
#define CXP_TRIGGER_DEBOUNCER__mode    Unknown
#define CXP_TRIGGER_DEBOUNCER__addr    0x8918

#define CXP_TRIGGER_SOFTWARE__kind    Value
#define CXP_TRIGGER_SOFTWARE__type    Enumeration
#define CXP_TRIGGER_SOFTWARE__size    4
#define CXP_TRIGGER_SOFTWARE__mode    WriteOnly
#define CXP_TRIGGER_SOFTWARE__addr    0x8910

#define CXP_TEST_IMAGE_SELECTOR__kind    Value
#define CXP_TEST_IMAGE_SELECTOR__type    Enumeration
#define CXP_TEST_IMAGE_SELECTOR__size    4
#define CXP_TEST_IMAGE_SELECTOR__mode    ReadWrite
#define CXP_TEST_IMAGE_SELECTOR__addr    0x9000

#define CXP_EXPOSURE_MODE__kind    Value
#define CXP_EXPOSURE_MODE__type    Enumeration
#define CXP_EXPOSURE_MODE__size    4
#define CXP_EXPOSURE_MODE__mode    ReadWrite
#define CXP_EXPOSURE_MODE__addr    0x8944

#define CXP_EXPOSURE_TIME__kind    Value
#define CXP_EXPOSURE_TIME__type    Integer
#define CXP_EXPOSURE_TIME__size    4
#define CXP_EXPOSURE_TIME__mode    ReadWrite
#define CXP_EXPOSURE_TIME__addr    0x8840
#define CXP_EXPOSURE_TIME__min     1
#define CXP_EXPOSURE_TIME__max     100000 // depends on ACQUISITION_FRAME_RATE

#define CXP_EXPOSURE_TIME_MAX__kind    Value
#define CXP_EXPOSURE_TIME_MAX__type    Integer
#define CXP_EXPOSURE_TIME_MAX__size    4
#define CXP_EXPOSURE_TIME_MAX__mode    ReadOnly
#define CXP_EXPOSURE_TIME_MAX__addr    0x8818

#define CXP_ACQUISITION_FRAME_RATE__kind    Value
#define CXP_ACQUISITION_FRAME_RATE__type    Integer
#define CXP_ACQUISITION_FRAME_RATE__size    4
#define CXP_ACQUISITION_FRAME_RATE__mode    ReadWrite
#define CXP_ACQUISITION_FRAME_RATE__addr    0x8814
#define CXP_ACQUISITION_FRAME_RATE__min     10 // FIXME: the XML file says 16
#define CXP_ACQUISITION_FRAME_RATE__max     1000000 // depends on EXPOSURE_TIME

#define CXP_ACQUISITION_FRAME_RATE_MAX__kind    Value
#define CXP_ACQUISITION_FRAME_RATE_MAX__type    Integer
#define CXP_ACQUISITION_FRAME_RATE_MAX__size    4
#define CXP_ACQUISITION_FRAME_RATE_MAX__mode    ReadOnly
#define CXP_ACQUISITION_FRAME_RATE_MAX__addr    0x881C

#define CXP_SEQUENCER_CONFIGURATION_MODE__kind    Value
#define CXP_SEQUENCER_CONFIGURATION_MODE__type    Integer
#define CXP_SEQUENCER_CONFIGURATION_MODE__size    4
#define CXP_SEQUENCER_CONFIGURATION_MODE__mode    Unknown
#define CXP_SEQUENCER_CONFIGURATION_MODE__addr    0x8874

#define CXP_SEQUENCER_MODE__kind    Value
#define CXP_SEQUENCER_MODE__type    Integer
#define CXP_SEQUENCER_MODE__size    4
#define CXP_SEQUENCER_MODE__mode    Unknown
#define CXP_SEQUENCER_MODE__addr    0x8870

#define CXP_SEQUENCER_SET_SELECTOR__kind    Value
#define CXP_SEQUENCER_SET_SELECTOR__type    Unknown
#define CXP_SEQUENCER_SET_SELECTOR__size    Unknown
#define CXP_SEQUENCER_SET_SELECTOR__mode    Unknown
#define CXP_SEQUENCER_SET_SELECTOR__addr    0x8878

#define CXP_SEQUENCER_SET_SAVE__kind    Value
#define CXP_SEQUENCER_SET_SAVE__type    Unknown
#define CXP_SEQUENCER_SET_SAVE__size    Unknown
#define CXP_SEQUENCER_SET_SAVE__mode    Unknown
#define CXP_SEQUENCER_SET_SAVE__addr    0x887C

#define CXP_SEQUENCER_SET_NEXT__kind    Value
#define CXP_SEQUENCER_SET_NEXT__type    Unknown
#define CXP_SEQUENCER_SET_NEXT__size    Unknown
#define CXP_SEQUENCER_SET_NEXT__mode    Unknown
#define CXP_SEQUENCER_SET_NEXT__addr    0x8888

#define CXP_USER_SET_SELECTOR__kind    Value
#define CXP_USER_SET_SELECTOR__type    Enumeration
#define CXP_USER_SET_SELECTOR__size    4
#define CXP_USER_SET_SELECTOR__mode    ReadWrite
#define CXP_USER_SET_SELECTOR__addr    0x8820

#define CXP_USER_SET_LOAD__kind    Value
#define CXP_USER_SET_LOAD__type    Command
#define CXP_USER_SET_LOAD__size    4
#define CXP_USER_SET_LOAD__mode    WriteOnly
#define CXP_USER_SET_LOAD__addr    0x8824
#define CXP_USER_SET_LOAD__comm    1

#define CXP_USER_SET_SAVE__kind    Value
#define CXP_USER_SET_SAVE__type    Command
#define CXP_USER_SET_SAVE__size    4
#define CXP_USER_SET_SAVE__mode    WriteOnly
#define CXP_USER_SET_SAVE__addr    0x8828
#define CXP_USER_SET_SAVE__comm    1

#define CXP_USER_SET_DEFAULT_SELECTOR__kind    Value
#define CXP_USER_SET_DEFAULT_SELECTOR__type    Enumeration
#define CXP_USER_SET_DEFAULT_SELECTOR__size    4
#define CXP_USER_SET_DEFAULT_SELECTOR__mode    ReadWrite
#define CXP_USER_SET_DEFAULT_SELECTOR__addr    0x882C

#define CXP_DEVICE_RESET__kind    Value
#define CXP_DEVICE_RESET__type    Command
#define CXP_DEVICE_RESET__size    4
#define CXP_DEVICE_RESET__mode    WriteOnly
#define CXP_DEVICE_RESET__addr    0x8300
#define CXP_DEVICE_RESET__comm    1

#define CXP_CONNECTION_DEFAULT__kind    Value
#define CXP_CONNECTION_DEFAULT__type    Integer
#define CXP_CONNECTION_DEFAULT__size    4
#define CXP_CONNECTION_DEFAULT__mode    ReadOnly
#define CXP_CONNECTION_DEFAULT__addr    0x4018

#define CXP_REGION_SELECTOR__kind    Value
#define CXP_REGION_SELECTOR__type    Enumeration
#define CXP_REGION_SELECTOR__size    4
#define CXP_REGION_SELECTOR__mode    ReadWrite
#define CXP_REGION_SELECTOR__addr    0x8180

#define CXP_REGION_MODE__kind    Value
#define CXP_REGION_MODE__type    Enumeration
#define CXP_REGION_MODE__size    4
#define CXP_REGION_MODE__mode    ReadWrite
#define CXP_REGION_MODE__addr    0x8184

#define CXP_REGION_DESTINATION__kind    Value
#define CXP_REGION_DESTINATION__type    Enumeration
#define CXP_REGION_DESTINATION__size    4
#define CXP_REGION_DESTINATION__mode    ReadWrite
#define CXP_REGION_DESTINATION__addr    0x8188 /* always 0x0, i.e. "Stream1" */

#define CXP_WIDTH__kind    Value
#define CXP_WIDTH__type    Integer
#define CXP_WIDTH__size    4
#define CXP_WIDTH__mode    ReadWrite
#define CXP_WIDTH__addr    0x8118

#define CXP_HEIGHT__kind    Value
#define CXP_HEIGHT__type    Integer
#define CXP_HEIGHT__size    4
#define CXP_HEIGHT__mode    ReadWrite
#define CXP_HEIGHT__addr    0x811c

#define CXP_OFFSET_X__kind    Value
#define CXP_OFFSET_X__type    Integer
#define CXP_OFFSET_X__size    4
#define CXP_OFFSET_X__mode    ReadWrite
#define CXP_OFFSET_X__addr    0x8800

#define CXP_OFFSET_Y__kind    Value
#define CXP_OFFSET_Y__type    Integer
#define CXP_OFFSET_Y__size    4
#define CXP_OFFSET_Y__mode    ReadWrite
#define CXP_OFFSET_Y__addr    0x8804

#define CXP_DECIMATION_HORIZONTAL__kind    Value
#define CXP_DECIMATION_HORIZONTAL__type    Integer
#define CXP_DECIMATION_HORIZONTAL__size    4
#define CXP_DECIMATION_HORIZONTAL__mode    ReadWrite
#define CXP_DECIMATION_HORIZONTAL__addr    0x8190

#define CXP_DECIMATION_VERTICAL__kind    Value
#define CXP_DECIMATION_VERTICAL__type    Integer
#define CXP_DECIMATION_VERTICAL__size    4
#define CXP_DECIMATION_VERTICAL__mode    ReadWrite
#define CXP_DECIMATION_VERTICAL__addr    0x818c

#define CXP_SENSOR_WIDTH__kind    Value
#define CXP_SENSOR_WIDTH__type    Integer
#define CXP_SENSOR_WIDTH__size    4
#define CXP_SENSOR_WIDTH__mode    ReadOnly
#define CXP_SENSOR_WIDTH__addr    0x8808

#define CXP_SENSOR_HEIGHT__kind    Value
#define CXP_SENSOR_HEIGHT__type    Integer
#define CXP_SENSOR_HEIGHT__size    4
#define CXP_SENSOR_HEIGHT__mode    ReadOnly
#define CXP_SENSOR_HEIGHT__addr    0x880c

#define CXP_TAP_GEOMETRY__kind    Value
#define CXP_TAP_GEOMETRY__type    Enumeration
#define CXP_TAP_GEOMETRY__size    4
#define CXP_TAP_GEOMETRY__mode    ReadOnly
#define CXP_TAP_GEOMETRY__addr    0x8160 // always 0x0, i.e. "Geometry_1X_1Y"

#define CXP_PIXEL_FORMAT__kind    Value
#define CXP_PIXEL_FORMAT__type    Enumeration
#define CXP_PIXEL_FORMAT__size    4
#define CXP_PIXEL_FORMAT__mode    ReadWrite
#define CXP_PIXEL_FORMAT__addr    0x8144

#define CXP_IMAGE1_STREAM_ID__kind    Value
#define CXP_IMAGE1_STREAM_ID__type    Integer
#define CXP_IMAGE1_STREAM_ID__size    4
#define CXP_IMAGE1_STREAM_ID__mode    ReadOnly
#define CXP_IMAGE1_STREAM_ID__addr    0x8164
// DEVICE_SCANTYPE = RegisterEnum{ReadOnly}(????) // always 0x0, i.e. "Areascan"

#define CXP_REGION_MODE_OFF    0x00
#define CXP_REGION_MODE_ON     0x01

#define CXP_PIXEL_FORMAT_MONO8        0x0101
#define CXP_PIXEL_FORMAT_MONO10       0x0102
#define CXP_PIXEL_FORMAT_BAYERGR8     0x0311
#define CXP_PIXEL_FORMAT_BAYERGR10    0x0312

#define CXP_REGION_SELECTOR_REGION0    0x0
#define CXP_REGION_SELECTOR_REGION1    0x1
#define CXP_REGION_SELECTOR_REGION2    0x2

#define CXP_GAIN__kind    Value
#define CXP_GAIN__type    Integer
#define CXP_GAIN__size    4
#define CXP_GAIN__mode    ReadWrite
#define CXP_GAIN__addr    0x8850
#define CXP_GAIN__min     50
#define CXP_GAIN__max     1000

#define CXP_BLACK_LEVEL__kind    Value
#define CXP_BLACK_LEVEL__type    Integer
#define CXP_BLACK_LEVEL__size    4
#define CXP_BLACK_LEVEL__mode    ReadWrite
#define CXP_BLACK_LEVEL__addr    0x8854
#define CXP_BLACK_LEVEL__min     0
#define CXP_BLACK_LEVEL__max     500
#define CXP_BLACK_LEVEL__inc     1

#define CXP_GAMMA__kind    Value
#define CXP_GAMMA__type    Float
#define CXP_GAMMA__size    4
#define CXP_GAMMA__mode    ReadWrite
#define CXP_GAMMA__addr    0x8858
#define CXP_GAMMA__min     0.1
#define CXP_GAMMA__max     3.0
#define CXP_GAMMA__inc     0.1
// LINE_SOURCE   = RegisterEnum{ReadWrite}(0x????)
// LINE_SELECTOR = RegisterEnum{ReadWrite}(0x????)

#define CXP_LINE_INVERTER__kind    Value
#define CXP_LINE_INVERTER__type    Enumeration
#define CXP_LINE_INVERTER__size    4
#define CXP_LINE_INVERTER__mode    ReadWrite
#define CXP_LINE_INVERTER__addr    0x8A20

#define CXP_TX_LOGICAL_CONNECTION_RESET__kind    Value
#define CXP_TX_LOGICAL_CONNECTION_RESET__type    Unknown
#define CXP_TX_LOGICAL_CONNECTION_RESET__size    Unknown
#define CXP_TX_LOGICAL_CONNECTION_RESET__mode    Unknown
#define CXP_TX_LOGICAL_CONNECTION_RESET__addr    0x9010

#define CXP_PRST_ENABLE__kind    Value
#define CXP_PRST_ENABLE__type    Integer
#define CXP_PRST_ENABLE__size    4
#define CXP_PRST_ENABLE__mode    ReadWrite
#define CXP_PRST_ENABLE__addr    0x9200

#define CXP_PULSE_DRAIN_ENABLE__kind    Value
#define CXP_PULSE_DRAIN_ENABLE__type    Integer
#define CXP_PULSE_DRAIN_ENABLE__size    4
#define CXP_PULSE_DRAIN_ENABLE__mode    ReadWrite
#define CXP_PULSE_DRAIN_ENABLE__addr    0x9204

#define CXP_CUSTOM_SENSOR_CLK_ENABLE__kind    Value
#define CXP_CUSTOM_SENSOR_CLK_ENABLE__type    Unknown
#define CXP_CUSTOM_SENSOR_CLK_ENABLE__size    Unknown
#define CXP_CUSTOM_SENSOR_CLK_ENABLE__mode    Unknown
#define CXP_CUSTOM_SENSOR_CLK_ENABLE__addr    0x9300

#define CXP_CUSTOM_SENSOR_CLK__kind    Value
#define CXP_CUSTOM_SENSOR_CLK__type    Unknown
#define CXP_CUSTOM_SENSOR_CLK__size    Unknown
#define CXP_CUSTOM_SENSOR_CLK__mode    Unknown
#define CXP_CUSTOM_SENSOR_CLK__addr    0x9304

#define CXP_DEVICE_INFORMATION__kind    Value
#define CXP_DEVICE_INFORMATION__type    Integer
#define CXP_DEVICE_INFORMATION__size    4
#define CXP_DEVICE_INFORMATION__mode    ReadOnly
#define CXP_DEVICE_INFORMATION__addr    0x8A04

#define CXP_DEVICE_INFORMATION_SELECTOR__kind    Value
#define CXP_DEVICE_INFORMATION_SELECTOR__type    Enumeration
#define CXP_DEVICE_INFORMATION_SELECTOR__size    4
#define CXP_DEVICE_INFORMATION_SELECTOR__mode    ReadWrite
#define CXP_DEVICE_INFORMATION_SELECTOR__addr    0x8A00

#define CXP_ANALOG_REGISTER_SET_SELECTOR__kind    Value
#define CXP_ANALOG_REGISTER_SET_SELECTOR__type    Unknown
#define CXP_ANALOG_REGISTER_SET_SELECTOR__size    Unknown
#define CXP_ANALOG_REGISTER_SET_SELECTOR__mode    Unknown
#define CXP_ANALOG_REGISTER_SET_SELECTOR__addr    0x20000

#define CXP_ANALOG_REGISTER_SELECTOR__kind    Value
#define CXP_ANALOG_REGISTER_SELECTOR__type    Unknown
#define CXP_ANALOG_REGISTER_SELECTOR__size    Unknown
#define CXP_ANALOG_REGISTER_SELECTOR__mode    Unknown
#define CXP_ANALOG_REGISTER_SELECTOR__addr    0x20004

#define CXP_ANALOG_VALUE__kind    Value
#define CXP_ANALOG_VALUE__type    Unknown
#define CXP_ANALOG_VALUE__size    Unknown
#define CXP_ANALOG_VALUE__mode    Unknown
#define CXP_ANALOG_VALUE__addr    0x20008

#define CXP_INFO_FIELD_FRAME_COUNTER_ENABLE__kind    Value
#define CXP_INFO_FIELD_FRAME_COUNTER_ENABLE__type    Integer
#define CXP_INFO_FIELD_FRAME_COUNTER_ENABLE__size    4
#define CXP_INFO_FIELD_FRAME_COUNTER_ENABLE__mode    ReadWrite
#define CXP_INFO_FIELD_FRAME_COUNTER_ENABLE__addr    0x9310

#define CXP_INFO_FIELD_TIME_STAMP_ENABLE__kind    Value
#define CXP_INFO_FIELD_TIME_STAMP_ENABLE__type    Integer
#define CXP_INFO_FIELD_TIME_STAMP_ENABLE__size    4
#define CXP_INFO_FIELD_TIME_STAMP_ENABLE__mode    ReadWrite
#define CXP_INFO_FIELD_TIME_STAMP_ENABLE__addr    0x9314

#define CXP_INFO_FIELD_ROI_ENABLE__kind    Value
#define CXP_INFO_FIELD_ROI_ENABLE__type    Integer
#define CXP_INFO_FIELD_ROI_ENABLE__size    4
#define CXP_INFO_FIELD_ROI_ENABLE__mode    ReadWrite
#define CXP_INFO_FIELD_ROI_ENABLE__addr    0x9318

#define CXP_FIXED_PATTERN_NOISE_REDUCTION__kind    Value
#define CXP_FIXED_PATTERN_NOISE_REDUCTION__type    Integer
#define CXP_FIXED_PATTERN_NOISE_REDUCTION__size    4
#define CXP_FIXED_PATTERN_NOISE_REDUCTION__mode    ReadWrite
#define CXP_FIXED_PATTERN_NOISE_REDUCTION__addr    0x8A10

#define CXP_FILTER_MODE__kind    Value
#define CXP_FILTER_MODE__type    Enumeration
#define CXP_FILTER_MODE__size    4
#define CXP_FILTER_MODE__mode    ReadWrite
#define CXP_FILTER_MODE__addr    0x10014

#define CXP_PIXEL_TYPE_F__kind    Value
#define CXP_PIXEL_TYPE_F__type    Unknown
#define CXP_PIXEL_TYPE_F__size    Unknown
#define CXP_PIXEL_TYPE_F__mode    Unknown
#define CXP_PIXEL_TYPE_F__addr    0x51004

#define CXP_DIN1_CONNECTOR_TYPE__kind    Value
#define CXP_DIN1_CONNECTOR_TYPE__type    Unknown
#define CXP_DIN1_CONNECTOR_TYPE__size    Unknown
#define CXP_DIN1_CONNECTOR_TYPE__mode    Unknown
#define CXP_DIN1_CONNECTOR_TYPE__addr    0x8A30

#define CXP_IS_IMPLEMENTED_MULTI_ROI__kind    Value
#define CXP_IS_IMPLEMENTED_MULTI_ROI__type    Unknown
#define CXP_IS_IMPLEMENTED_MULTI_ROI__size    Unknown
#define CXP_IS_IMPLEMENTED_MULTI_ROI__mode    Unknown
#define CXP_IS_IMPLEMENTED_MULTI_ROI__addr    0x50004

#define CXP_IS_IMPLEMENTED_SEQUENCER__kind    Value
#define CXP_IS_IMPLEMENTED_SEQUENCER__type    Unknown
#define CXP_IS_IMPLEMENTED_SEQUENCER__size    Unknown
#define CXP_IS_IMPLEMENTED_SEQUENCER__mode    Unknown
#define CXP_IS_IMPLEMENTED_SEQUENCER__addr    0x50008

#define CXP_CAMERA_TYPE_HEX__kind    Value
#define CXP_CAMERA_TYPE_HEX__type    Unknown
#define CXP_CAMERA_TYPE_HEX__size    Unknown
#define CXP_CAMERA_TYPE_HEX__mode    Unknown
#define CXP_CAMERA_TYPE_HEX__addr    0x51000

#define CXP_CAMERA_STATUS__kind    Value
#define CXP_CAMERA_STATUS__type    Unknown
#define CXP_CAMERA_STATUS__size    Unknown
#define CXP_CAMERA_STATUS__mode    Unknown
#define CXP_CAMERA_STATUS__addr    0x10002200

#define CXP_IS_STOPPED__kind    Value
#define CXP_IS_STOPPED__type    Unknown
#define CXP_IS_STOPPED__size    Unknown
#define CXP_IS_STOPPED__mode    Unknown
#define CXP_IS_STOPPED__addr    0x10002204

/* Image format control.
 *
 * WidthMax  = ((div(SENSOR_WIDTH  - OFFSET_X), 16)*16)
 * HeightMax = ((div(SENSOR_HEIGHT - OFFSET_Y),  2)* 2)
 */
#define CXP_HORIZONTAL_INCREMENT    16
#define CXP_VERTICAL_INCREMENT       2

/* Image filter modes. */
#define CXP_FILTER_MODE_RAW     0
#define CXP_FILTER_MODE_MONO    1
#define CXP_FILTER_MODE_COLOR   2

/* Possible values for  CXP_DEVICE_INFORMATION_SELECTOR. */
#define CXP_DEVICE_INFORMATION_SELECTOR_SERIAL_NUMBER      0
#define CXP_DEVICE_INFORMATION_SELECTOR_DEVICE_TYPE        1
#define CXP_DEVICE_INFORMATION_SELECTOR_DEVICE_SUBTYPE     2
#define CXP_DEVICE_INFORMATION_SELECTOR_HARDWARE_REVISION  3
#define CXP_DEVICE_INFORMATION_SELECTOR_FPGA_VERSION       4
#define CXP_DEVICE_INFORMATION_SELECTOR_SOFTWARE_VERSION   5
#define CXP_DEVICE_INFORMATION_SELECTOR_POWER_SOURCE      20
#define CXP_DEVICE_INFORMATION_SELECTOR_POWER_CONSUMPTION 21
#define CXP_DEVICE_INFORMATION_SELECTOR_POWER_VOLTAGE     22
#define CXP_DEVICE_INFORMATION_SELECTOR_TEMPERATURE       23

#endif /* _MIKROTRON_MC408X_H */
