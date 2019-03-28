/*
 * andor-features.h --
 *
 * Features definitions for Andor cameras.
 *
 *-----------------------------------------------------------------------------
 *
 * This file if part of the TAO library (https://github.com/emmt/TAO) licensed
 * under the MIT license.
 *
 * Copyright (C) 2019, Éric Thiébaut.
 */

#ifndef _ANDOR_FEATURES_H
#define _ANDOR_FEATURES_H 1

#include "andor.h"

/*
 * All known features (i.e. found in Andor SDK Documentation) are summarized in
 * the following table stored in the macro `_ANDOR_FEATURES`.  The idea is to *
 * (re)define the macro `_ANDOR_FEATURE` to format the table or extract
 * whatever field is needed.  The `_ANDOR_FEATURE` macro is supposed to have as
 * many arguments at the number of columns of the table:
 *
 *     #define _ANDOR_FEATURE(key, sys, sim, zyl)
 *
 * The first column is the symbolic feature name, 2nd column is for the system
 * handle, 3rd column is for the SimCam, 4th column is for the Zyla.  Other
 * cameras will be added later.  The letters in the 2nd and subsequent columns
 * gives the type of the feature for the considered camera:
 *
 *     B = Boolean
 *     C = Command
 *     I = Integer
 *     F = Floating-point
 *     E = Enumerated
 *     S = String
 *     X = Not implemented
 */
#define _ANDOR_FEATURES \
    _ANDOR_FEATURE(AOIBinning,                  X, X, E) \
    _ANDOR_FEATURE(AOIHBin,                     X, I, I) \
    _ANDOR_FEATURE(AOIHeight,                   X, I, I) \
    _ANDOR_FEATURE(AOILayout,                   X, X, E) \
    _ANDOR_FEATURE(AOILeft,                     X, I, I) \
    _ANDOR_FEATURE(AOIStride,                   X, I, I) \
    _ANDOR_FEATURE(AOITop,                      X, I, I) \
    _ANDOR_FEATURE(AOIVBin,                     X, I, I) \
    _ANDOR_FEATURE(AOIWidth,                    X, I, I) \
    _ANDOR_FEATURE(AccumulateCount,             X, X, I) \
    _ANDOR_FEATURE(AcquiredCount,               X, X, X) \
    _ANDOR_FEATURE(AcquisitionStart,            X, C, C) \
    _ANDOR_FEATURE(AcquisitionStop,             X, C, C) \
    _ANDOR_FEATURE(AlternatingReadoutDirection, X, X, B) \
    _ANDOR_FEATURE(AuxOutSourceTwo,             X, X, E) \
    _ANDOR_FEATURE(AuxiliaryOutSource,          X, X, E) \
    _ANDOR_FEATURE(BackoffTemperatureOffset,    X, X, X) \
    _ANDOR_FEATURE(Baseline,                    X, X, I) \
    _ANDOR_FEATURE(BitDepth,                    X, X, E) \
    _ANDOR_FEATURE(BufferOverflowEvent,         X, X, I) \
    _ANDOR_FEATURE(BytesPerPixel,               X, X, I) \
    _ANDOR_FEATURE(CameraAcquiring,             X, B, B) \
    _ANDOR_FEATURE(CameraFamily,                X, X, X) \
    _ANDOR_FEATURE(CameraMemory,                X, X, X) \
    _ANDOR_FEATURE(CameraModel,                 X, S, S) \
    _ANDOR_FEATURE(CameraName,                  X, X, S) \
    _ANDOR_FEATURE(CameraPresent,               X, X, B) \
    _ANDOR_FEATURE(ColourFilter,                X, X, X) \
    _ANDOR_FEATURE(ControllerID,                X, X, S) \
    _ANDOR_FEATURE(CoolerPower,                 X, X, F) \
    _ANDOR_FEATURE(CycleMode,                   X, E, E) \
    _ANDOR_FEATURE(DDGIOCEnable,                X, X, X) \
    _ANDOR_FEATURE(DDGIOCNumberOfPulses,        X, X, X) \
    _ANDOR_FEATURE(DDGIOCPeriod,                X, X, X) \
    _ANDOR_FEATURE(DDGOpticalWidthEnable,       X, X, X) \
    _ANDOR_FEATURE(DDGOutputDelay,              X, X, X) \
    _ANDOR_FEATURE(DDGOutputEnable,             X, X, X) \
    _ANDOR_FEATURE(DDGOutputPolarity,           X, X, X) \
    _ANDOR_FEATURE(DDGOutputSelector,           X, X, X) \
    _ANDOR_FEATURE(DDGOutputStepEnable,         X, X, X) \
    _ANDOR_FEATURE(DDGOutputWidth,              X, X, X) \
    _ANDOR_FEATURE(DDGStepCount,                X, X, X) \
    _ANDOR_FEATURE(DDGStepDelayCoefficientA,    X, X, X) \
    _ANDOR_FEATURE(DDGStepDelayCoefficientB,    X, X, X) \
    _ANDOR_FEATURE(DDGStepDelayMode,            X, X, X) \
    _ANDOR_FEATURE(DDGStepEnabled,              X, X, X) \
    _ANDOR_FEATURE(DDGStepUploadModeValues,     X, X, X) \
    _ANDOR_FEATURE(DDGStepUploadProgress,       X, X, X) \
    _ANDOR_FEATURE(DDGStepUploadRequired,       X, X, X) \
    _ANDOR_FEATURE(DDGStepWidthCoefficientA,    X, X, X) \
    _ANDOR_FEATURE(DDGStepWidthCoefficientB,    X, X, X) \
    _ANDOR_FEATURE(DDGStepWidthMode,            X, X, X) \
    _ANDOR_FEATURE(DDR2Type,                    X, X, X) \
    _ANDOR_FEATURE(DeviceCount,                 I, X, X) \
    _ANDOR_FEATURE(DeviceVideoIndex,            X, X, I) \
    _ANDOR_FEATURE(DisableShutter,              X, X, X) \
    _ANDOR_FEATURE(DriverVersion,               X, X, X) \
    _ANDOR_FEATURE(ElectronicShutteringMode,    X, E, E) \
    _ANDOR_FEATURE(EventEnable,                 X, X, B) \
    _ANDOR_FEATURE(EventSelector,               X, X, E) \
    _ANDOR_FEATURE(EventsMissedEvent,           X, X, I) \
    _ANDOR_FEATURE(ExposedPixelHeight,          X, X, I) \
    _ANDOR_FEATURE(ExposureEndEvent,            X, X, I) \
    _ANDOR_FEATURE(ExposureStartEvent,          X, X, I) \
    _ANDOR_FEATURE(ExposureTime,                X, F, F) \
    _ANDOR_FEATURE(ExternalIOReadout,           X, X, X) \
    _ANDOR_FEATURE(ExternalTriggerDelay,        X, X, F) \
    _ANDOR_FEATURE(FanSpeed,                    X, E, E) \
    _ANDOR_FEATURE(FastAOIFrameRateEnable,      X, X, B) \
    _ANDOR_FEATURE(FirmwareVersion,             X, X, S) \
    _ANDOR_FEATURE(ForceShutterOpen,            X, X, X) \
    _ANDOR_FEATURE(FrameCount,                  X, I, I) \
    _ANDOR_FEATURE(FrameGenFixedPixelValue,     X, X, X) \
    _ANDOR_FEATURE(FrameGenMode,                X, X, X) \
    _ANDOR_FEATURE(FrameInterval,               X, X, X) \
    _ANDOR_FEATURE(FrameIntervalTiming,         X, X, X) \
    _ANDOR_FEATURE(FrameRate,                   X, F, F) \
    _ANDOR_FEATURE(FullAOIControl,              X, X, B) \
    _ANDOR_FEATURE(GateMode,                    X, X, X) \
    _ANDOR_FEATURE(HeatSinkTemperature,         X, X, X) \
    _ANDOR_FEATURE(I2CAddress,                  X, X, X) \
    _ANDOR_FEATURE(I2CByte,                     X, X, X) \
    _ANDOR_FEATURE(I2CByteCount,                X, X, X) \
    _ANDOR_FEATURE(I2CByteSelector,             X, X, X) \
    _ANDOR_FEATURE(I2CRead,                     X, X, X) \
    _ANDOR_FEATURE(I2CWrite,                    X, X, X) \
    _ANDOR_FEATURE(IOControl,                   X, X, X) \
    _ANDOR_FEATURE(IODirection,                 X, X, X) \
    _ANDOR_FEATURE(IOInvert,                    X, X, B) \
    _ANDOR_FEATURE(IOSelector,                  X, X, E) \
    _ANDOR_FEATURE(IOState,                     X, X, X) \
    _ANDOR_FEATURE(IRPreFlashEnable,            X, X, X) \
    _ANDOR_FEATURE(ImageSizeBytes,              X, I, I) \
    _ANDOR_FEATURE(InputVoltage,                X, X, X) \
    _ANDOR_FEATURE(InsertionDelay,              X, X, X) \
    _ANDOR_FEATURE(InterfaceType,               X, X, S) \
    _ANDOR_FEATURE(KeepCleanEnable,             X, X, X) \
    _ANDOR_FEATURE(KeepCleanPostExposureEnable, X, X, X) \
    _ANDOR_FEATURE(LUTIndex,                    X, X, I) \
    _ANDOR_FEATURE(LUTValue,                    X, X, I) \
    _ANDOR_FEATURE(LineScanSpeed,               X, X, F) \
    _ANDOR_FEATURE(LogLevel,                    X, E, E) \
    _ANDOR_FEATURE(MCPGain,                     X, X, X) \
    _ANDOR_FEATURE(MCPIntelligate,              X, X, X) \
    _ANDOR_FEATURE(MCPVoltage,                  X, X, X) \
    _ANDOR_FEATURE(MaxInterfaceTransferRate,    X, X, F) \
    _ANDOR_FEATURE(MetadataEnable,              X, X, B) \
    _ANDOR_FEATURE(MetadataFrame,               X, X, B) \
    _ANDOR_FEATURE(MetadataTimestamp,           X, X, B) \
    _ANDOR_FEATURE(MicrocodeVersion,            X, X, X) \
    _ANDOR_FEATURE(MultitrackBinned,            X, X, B) \
    _ANDOR_FEATURE(MultitrackCount,             X, X, I) \
    _ANDOR_FEATURE(MultitrackEnd,               X, X, I) \
    _ANDOR_FEATURE(MultitrackSelector,          X, X, I) \
    _ANDOR_FEATURE(MultitrackStart,             X, X, I) \
    _ANDOR_FEATURE(Overlap,                     X, X, B) \
    _ANDOR_FEATURE(PIVEnable,                   X, X, X) \
    _ANDOR_FEATURE(PixelCorrection,             X, E, X) \
    _ANDOR_FEATURE(PixelEncoding,               X, E, E) \
    _ANDOR_FEATURE(PixelHeight,                 X, F, F) \
    _ANDOR_FEATURE(PixelReadoutRate,            X, E, E) \
    _ANDOR_FEATURE(PixelWidth,                  X, F, F) \
    _ANDOR_FEATURE(PortSelector,                X, X, X) \
    _ANDOR_FEATURE(PreAmpGain,                  X, E, X) \
    _ANDOR_FEATURE(PreAmpGainChannel,           X, E, X) \
    _ANDOR_FEATURE(PreAmpGainControl,           X, X, X) \
    _ANDOR_FEATURE(PreAmpGainSelector,          X, E, X) \
    _ANDOR_FEATURE(PreAmpGainValue,             X, X, X) \
    _ANDOR_FEATURE(PreAmpOffsetValue,           X, X, X) \
    _ANDOR_FEATURE(PreTriggerEnable,            X, X, X) \
    _ANDOR_FEATURE(ReadoutTime,                 X, X, F) \
    _ANDOR_FEATURE(RollingShutterGlobalClear,   X, X, B) \
    _ANDOR_FEATURE(RowNExposureEndEvent,        X, X, I) \
    _ANDOR_FEATURE(RowNExposureStartEvent,      X, X, I) \
    _ANDOR_FEATURE(RowReadTime,                 X, X, F) \
    _ANDOR_FEATURE(ScanSpeedControlEnable,      X, X, B) \
    _ANDOR_FEATURE(SensorCooling,               X, B, B) \
    _ANDOR_FEATURE(SensorHeight,                X, I, I) \
    _ANDOR_FEATURE(SensorModel,                 X, X, X) \
    _ANDOR_FEATURE(SensorReadoutMode,           X, X, E) \
    _ANDOR_FEATURE(SensorTemperature,           X, F, F) \
    _ANDOR_FEATURE(SensorType,                  X, X, X) \
    _ANDOR_FEATURE(SensorWidth,                 X, I, I) \
    _ANDOR_FEATURE(SerialNumber,                X, S, S) \
    _ANDOR_FEATURE(ShutterAmpControl,           X, X, X) \
    _ANDOR_FEATURE(ShutterMode,                 X, X, E) \
    _ANDOR_FEATURE(ShutterOutputMode,           X, X, E) \
    _ANDOR_FEATURE(ShutterState,                X, X, X) \
    _ANDOR_FEATURE(ShutterStrobePeriod,         X, X, X) \
    _ANDOR_FEATURE(ShutterStrobePosition,       X, X, X) \
    _ANDOR_FEATURE(ShutterTransferTime,         X, X, F) \
    _ANDOR_FEATURE(SimplePreAmpGainControl,     X, X, E) \
    _ANDOR_FEATURE(SoftwareTrigger,             X, X, C) \
    _ANDOR_FEATURE(SoftwareVersion,             S, X, X) \
    _ANDOR_FEATURE(SpuriousNoiseFilter,         X, X, B) \
    _ANDOR_FEATURE(StaticBlemishCorrection,     X, X, B) \
    _ANDOR_FEATURE(SynchronousTriggering,       X, B, X) \
    _ANDOR_FEATURE(TargetSensorTemperature,     X, F, X) \
    _ANDOR_FEATURE(TemperatureControl,          X, X, E) \
    _ANDOR_FEATURE(TemperatureStatus,           X, X, E) \
    _ANDOR_FEATURE(TimestampClock,              X, X, I) \
    _ANDOR_FEATURE(TimestampClockFrequency,     X, X, I) \
    _ANDOR_FEATURE(TimestampClockReset,         X, X, C) \
    _ANDOR_FEATURE(TransmitFrames,              X, X, X) \
    _ANDOR_FEATURE(TriggerMode,                 X, E, E) \
    _ANDOR_FEATURE(UsbDeviceId,                 X, X, X) \
    _ANDOR_FEATURE(UsbProductId,                X, X, X) \
    _ANDOR_FEATURE(VerticallyCentreAOI,         X, X, B)

_TAO_BEGIN_DECLS

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(key, sys, sim, zyl) key,
typedef enum andor_feature andor_feature_t;
enum andor_feature {_ANDOR_FEATURES};

#define ANDOR_NFEATURES (1 + VerticallyCentreAOI)

extern andor_feature_type_t andor_get_feature_type(andor_camera_t* cam,
                                                   andor_feature_t key,
                                                   unsigned int* mode);
extern andor_feature_type_t _andor_get_feature_type(AT_H handle,
                                                    const AT_WC* key,
                                                    unsigned int* mode);

/* Low-level getters and setters to correctly report errors. */

extern int _andor_is_implemented(tao_error_t** errs, AT_H handle,
                                 andor_feature_t key, bool* ptr,
                                 const char* info);
extern int _andor_is_readable(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, bool* ptr,
                              const char* info);
extern int _andor_is_writable(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, bool* ptr,
                              const char* info);
extern int _andor_is_readonly(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, bool* ptr,
                              const char* info);
extern int _andor_set_boolean(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, bool val,
                              const char* info);
extern int _andor_get_boolean(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, bool* ptr,
                              const char* info);
extern int _andor_set_integer(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, long val,
                              const char* info);
extern int _andor_get_integer(tao_error_t** errs, AT_H handle,
                              andor_feature_t key, long* ptr,
                              const char* info);
extern int _andor_get_integer_min(tao_error_t** errs, AT_H handle,
                                  andor_feature_t key, long* ptr,
                                  const char* info);
extern int _andor_get_integer_max(tao_error_t** errs, AT_H handle,
                                  andor_feature_t key, long* ptr,
                                  const char* info);
extern int _andor_set_float(tao_error_t** errs, AT_H handle,
                            andor_feature_t key, double val,
                            const char* info);
extern int _andor_get_float(tao_error_t** errs, AT_H handle,
                            andor_feature_t key, double* ptr,
                            const char* info);
extern int _andor_get_float_min(tao_error_t** errs, AT_H handle,
                                andor_feature_t key, double* ptr,
                                const char* info);
extern int _andor_get_float_max(tao_error_t** errs, AT_H handle,
                                andor_feature_t key, double* ptr,
                                const char* info);
extern int _andor_set_enum_index(tao_error_t** errs, AT_H handle,
                                 andor_feature_t key, int val,
                                 const char* info);
extern int _andor_set_enum_string(tao_error_t** errs, AT_H handle,
                                  andor_feature_t key, const AT_WC* val,
                                  const char* info);
extern int _andor_get_enum_index(tao_error_t** errs, AT_H handle,
                                 andor_feature_t key, int* ptr,
                                 const char* info);
extern int _andor_get_enum_count(tao_error_t** errs, AT_H handle,
                                 andor_feature_t key, int* ptr,
                                 const char* info);
extern int _andor_is_enum_index_available(tao_error_t** errs, AT_H handle,
                                          andor_feature_t key, int idx,
                                          bool* ptr, const char* info);
extern int _andor_is_enum_index_implemented(tao_error_t** errs, AT_H handle,
                                            andor_feature_t key, int idx,
                                            bool* ptr, const char* info);
extern int _andor_set_string(tao_error_t** errs, AT_H handle,
                             andor_feature_t key, const AT_WC* val,
                             const char* info);
extern int _andor_get_string(tao_error_t** errs, AT_H handle,
                             andor_feature_t key, AT_WC* str, long len,
                             const char* info);
extern int _andor_get_string_max_length(tao_error_t** errs, AT_H handle,
                                        andor_feature_t key, long* ptr,
                                        const char* info);

_TAO_END_DECLS

/* The following macros are needed when no `andor_camera_t` structure is
   available. */

#define ANDOR_GET_BOOLEAN0(errs, handle, key, ptr) \
    _andor_get_boolean(errs, handle, key, ptr, "AT_GetBool("#key")")

#define ANDOR_GET_INTEGER0(errs, handle, key, ptr) \
    _andor_get_integer(errs, handle, key, ptr, "AT_GetInt("#key")")

#define ANDOR_GET_FLOAT0(errs, handle, key, ptr) \
    _andor_get_float(errs, handle, key, ptr, "AT_GetFloat("#key")")

#define ANDOR_GET_STRING_MAX_LENGTH0(errs, handle, key, ptr)            \
    _andor_get_string_max_length(errs, handle, key, ptr,                \
                                 "AT_GetStringMaxLength("#key")")

#define ANDOR_GET_STRING0(errs, handle, key, ptr, len) \
    _andor_get_string(errs, handle, key, ptr, len, "AT_GetString("#key")")


/* The following macros work with a `andor_camera_t` structure. */

#define ANDOR_IS_IMPLEMENTED(cam, key, ptr)                     \
    _andor_is_implemented(&cam->errs, cam->handle, key, ptr,    \
                          "AT_IsImplemented("#key")")

#define ANDOR_IS_READABLE(cam, key, ptr)                        \
    _andor_is_readable(&cam->errs, cam->handle, key, ptr,       \
                       "AT_IsReadable("#key")")

#define ANDOR_IS_WRITABLE(cam, key, ptr)                        \
    _andor_is_writable(&cam->errs, cam->handle, key, ptr,       \
                       "AT_IsWritable("#key")")

#define ANDOR_IS_READONLY(cam, key, ptr)                        \
    _andor_is_readonly(&cam->errs, cam->handle, key, ptr,       \
                       "AT_IsReadOnly("#key")")

#define ANDOR_SET_BOOLEAN(cam, key, val)                        \
    _andor_set_boolean(&cam->errs, cam->handle, key, val,       \
                       "AT_SetBool("#key")")

#define ANDOR_GET_BOOLEAN(cam, key, ptr)                        \
    _andor_get_boolean(&cam->errs, cam->handle, key, ptr,       \
                       "AT_GetBool("#key")")

#define ANDOR_SET_INTEGER(cam, key, val)                        \
    _andor_set_integer(&cam->errs, cam->handle, key, val,       \
                       "AT_SetInt("#key")")

#define ANDOR_GET_INTEGER(cam, key, ptr)                        \
    _andor_get_integer(&cam->errs, cam->handle, key, ptr,       \
                       "AT_GetInt("#key")")

#define ANDOR_GET_INTEGER_MIN(cam, key, ptr)                    \
    _andor_get_integer_min(&cam->errs, cam->handle, key, ptr,   \
                           "AT_GetIntMin("#key")")

#define ANDOR_GET_INTEGER_MAX(cam, key, ptr)                    \
    _andor_get_integer_max(&cam->errs, cam->handle, key, ptr,   \
                           "AT_GetIntMax("#key")")

#define ANDOR_SET_FLOAT(cam, key, val)                  \
    _andor_set_float(&cam->errs, cam->handle, key, val, \
                     "AT_SetFloat("#key")")

#define ANDOR_GET_FLOAT(cam, key, ptr)                  \
    _andor_get_float(&cam->errs, cam->handle, key, ptr, \
                     "AT_GetFloat("#key")")

#define ANDOR_GET_FLOAT_MIN(cam, key, ptr)                      \
    _andor_get_float_min(&cam->errs, cam->handle, key, ptr,     \
                         "AT_GetFloatMin("#key")")

#define ANDOR_GET_FLOAT_MAX(cam, key, ptr)                      \
    _andor_get_float_max(&cam->errs, cam->handle, key, ptr,     \
                         "AT_GetFloatMax("#key")")

#define ANDOR_SET_ENUM_INDEX(cam, key, val)                     \
    _andor_set_enum_index(&cam->errs, cam->handle, key, val,    \
                          "AT_SetEnumIndex("#key")")

#define ANDOR_SET_ENUM_STRING(cam, key, val)                    \
    _andor_set_enum_string(&cam->errs, cam->handle, key, val,   \
                           "AT_SetEnumString("#key")")

#define ANDOR_GET_ENUM_INDEX(cam, key, ptr)                     \
    _andor_get_enum_index(&cam->errs, cam->handle, key, ptr,    \
                          "AT_GetEnumIndex("#key")")

#define ANDOR_GET_ENUM_COUNT(cam, key, ptr)                     \
    _andor_get_enum_count(&cam->errs, cam->handle, key, ptr,    \
                          "AT_GetEnumCount("#key")")

#define ANDOR_IS_ENUM_INDEX_AVAILABLE(cam, key, idx, ptr)               \
    _andor_is_enum_index_available(&cam->errs, cam->handle,             \
                                   key, idx, ptr,                       \
                                   "AT_IsEnumIndexAvailable("#key")")

#define ANDOR_IS_ENUM_INDEX_IMPLEMENTED(cam, key, idx, ptr)             \
    _andor_is_enum_index_implemented(&cam->errs, cam->handle,           \
                                     key, idx, ptr,                     \
                                     "AT_IsEnumIndexImplemented("#key")")

#define ANDOR_SET_STRING(cam, key, ptr)                         \
    _andor_set_string(&cam->errs, cam->handle, key, ptr, len,   \
                      "AT_SetString("#key")")

#define ANDOR_GET_STRING(cam, key, ptr, len)                    \
    _andor_get_string(&cam->errs, cam->handle, key, ptr, len,   \
                      "AT_GetString("#key")")

#define ANDOR_GET_STRING_MAX_LENGTH(cam, key, ptr)                      \
    _andor_get_string_max_length(&cam->errs, cam->handle, key, ptr,     \
                                 "AT_GetStringMaxLength("#key")")

#endif /* _ANDOR_FEATURES_H */
