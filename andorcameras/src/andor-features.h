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

#include "andorcameras.h"

/*
 * B = Boolean,
 * C = Command,
 * I = Integer,
 * F = Floating-point,
 * E = Enumerated,
 * S = String,
 * X = Not implemented
 *
 * First column is feature name, 2nd column is for the SimCam, 3rd column is
 * for the Zyla.
 */
#define _ANDOR_FEATURES \
    _ANDOR_FEATURE(AOIBinning,                  X, E) \
    _ANDOR_FEATURE(AOIHBin,                     I, I) \
    _ANDOR_FEATURE(AOIHeight,                   I, I) \
    _ANDOR_FEATURE(AOILayout,                   X, E) \
    _ANDOR_FEATURE(AOILeft,                     I, I) \
    _ANDOR_FEATURE(AOIStride,                   X, I) \
    _ANDOR_FEATURE(AOITop,                      I, I) \
    _ANDOR_FEATURE(AOIVBin,                     I, I) \
    _ANDOR_FEATURE(AOIWidth,                    I, I) \
    _ANDOR_FEATURE(AccumulateCount,             X, I) \
    _ANDOR_FEATURE(AcquiredCount,               X, X) \
    _ANDOR_FEATURE(AcquisitionStart,            C, C) \
    _ANDOR_FEATURE(AcquisitionStop,             C, C) \
    _ANDOR_FEATURE(AlternatingReadoutDirection, X, B) \
    _ANDOR_FEATURE(AuxOutSourceTwo,             X, E) \
    _ANDOR_FEATURE(AuxiliaryOutSource,          X, E) \
    _ANDOR_FEATURE(BackoffTemperatureOffset,    X, X) \
    _ANDOR_FEATURE(Baseline,                    X, I) \
    _ANDOR_FEATURE(BitDepth,                    X, E) \
    _ANDOR_FEATURE(BufferOverflowEvent,         X, I) \
    _ANDOR_FEATURE(BytesPerPixel,               X, I) \
    _ANDOR_FEATURE(CameraAcquiring,             B, B) \
    _ANDOR_FEATURE(CameraFamily,                X, X) \
    _ANDOR_FEATURE(CameraMemory,                X, X) \
    _ANDOR_FEATURE(CameraModel,                 S, S) \
    _ANDOR_FEATURE(CameraName,                  X, S) \
    _ANDOR_FEATURE(CameraPresent,               X, B) \
    _ANDOR_FEATURE(ColourFilter,                X, X) \
    _ANDOR_FEATURE(ControllerID,                X, S) \
    _ANDOR_FEATURE(CoolerPower,                 X, F) \
    _ANDOR_FEATURE(CycleMode,                   E, E) \
    _ANDOR_FEATURE(DDGIOCEnable,                X, X) \
    _ANDOR_FEATURE(DDGIOCNumberOfPulses,        X, X) \
    _ANDOR_FEATURE(DDGIOCPeriod,                X, X) \
    _ANDOR_FEATURE(DDGOpticalWidthEnable,       X, X) \
    _ANDOR_FEATURE(DDGOutputDelay,              X, X) \
    _ANDOR_FEATURE(DDGOutputEnable,             X, X) \
    _ANDOR_FEATURE(DDGOutputPolarity,           X, X) \
    _ANDOR_FEATURE(DDGOutputSelector,           X, X) \
    _ANDOR_FEATURE(DDGOutputStepEnable,         X, X) \
    _ANDOR_FEATURE(DDGOutputWidth,              X, X) \
    _ANDOR_FEATURE(DDGStepCount,                X, X) \
    _ANDOR_FEATURE(DDGStepDelayCoefficientA,    X, X) \
    _ANDOR_FEATURE(DDGStepDelayCoefficientB,    X, X) \
    _ANDOR_FEATURE(DDGStepDelayMode,            X, X) \
    _ANDOR_FEATURE(DDGStepEnabled,              X, X) \
    _ANDOR_FEATURE(DDGStepUploadModeValues,     X, X) \
    _ANDOR_FEATURE(DDGStepUploadProgress,       X, X) \
    _ANDOR_FEATURE(DDGStepUploadRequired,       X, X) \
    _ANDOR_FEATURE(DDGStepWidthCoefficientA,    X, X) \
    _ANDOR_FEATURE(DDGStepWidthCoefficientB,    X, X) \
    _ANDOR_FEATURE(DDGStepWidthMode,            X, X) \
    _ANDOR_FEATURE(DDR2Type,                    X, X) \
    _ANDOR_FEATURE(DeviceCount,                 I, I) \
    _ANDOR_FEATURE(DeviceVideoIndex,            X, I) \
    _ANDOR_FEATURE(DisableShutter,              X, X) \
    _ANDOR_FEATURE(DriverVersion,               X, X) \
    _ANDOR_FEATURE(ElectronicShutteringMode,    E, E) \
    _ANDOR_FEATURE(EventEnable,                 X, B) \
    _ANDOR_FEATURE(EventSelector,               X, E) \
    _ANDOR_FEATURE(EventsMissedEvent,           X, I) \
    _ANDOR_FEATURE(ExposedPixelHeight,          X, I) \
    _ANDOR_FEATURE(ExposureEndEvent,            X, I) \
    _ANDOR_FEATURE(ExposureStartEvent,          X, I) \
    _ANDOR_FEATURE(ExposureTime,                F, F) \
    _ANDOR_FEATURE(ExternalIOReadout,           X, X) \
    _ANDOR_FEATURE(ExternalTriggerDelay,        X, F) \
    _ANDOR_FEATURE(FanSpeed,                    E, E) \
    _ANDOR_FEATURE(FastAOIFrameRateEnable,      X, B) \
    _ANDOR_FEATURE(FirmwareVersion,             X, S) \
    _ANDOR_FEATURE(ForceShutterOpen,            X, X) \
    _ANDOR_FEATURE(FrameCount,                  I, I) \
    _ANDOR_FEATURE(FrameGenFixedPixelValue,     X, X) \
    _ANDOR_FEATURE(FrameGenMode,                X, X) \
    _ANDOR_FEATURE(FrameInterval,               X, X) \
    _ANDOR_FEATURE(FrameIntervalTiming,         X, X) \
    _ANDOR_FEATURE(FrameRate,                   F, F) \
    _ANDOR_FEATURE(FullAOIControl,              X, B) \
    _ANDOR_FEATURE(GateMode,                    X, X) \
    _ANDOR_FEATURE(HeatSinkTemperature,         X, X) \
    _ANDOR_FEATURE(I2CAddress,                  X, X) \
    _ANDOR_FEATURE(I2CByte,                     X, X) \
    _ANDOR_FEATURE(I2CByteCount,                X, X) \
    _ANDOR_FEATURE(I2CByteSelector,             X, X) \
    _ANDOR_FEATURE(I2CRead,                     X, X) \
    _ANDOR_FEATURE(I2CWrite,                    X, X) \
    _ANDOR_FEATURE(IOControl,                   X, X) \
    _ANDOR_FEATURE(IODirection,                 X, X) \
    _ANDOR_FEATURE(IOInvert,                    X, B) \
    _ANDOR_FEATURE(IOSelector,                  X, E) \
    _ANDOR_FEATURE(IOState,                     X, X) \
    _ANDOR_FEATURE(IRPreFlashEnable,            X, X) \
    _ANDOR_FEATURE(ImageSizeBytes,              I, I) \
    _ANDOR_FEATURE(InputVoltage,                X, X) \
    _ANDOR_FEATURE(InsertionDelay,              X, X) \
    _ANDOR_FEATURE(InterfaceType,               X, S) \
    _ANDOR_FEATURE(KeepCleanEnable,             X, X) \
    _ANDOR_FEATURE(KeepCleanPostExposureEnable, X, X) \
    _ANDOR_FEATURE(LUTIndex,                    X, I) \
    _ANDOR_FEATURE(LUTValue,                    X, I) \
    _ANDOR_FEATURE(LineScanSpeed,               X, F) \
    _ANDOR_FEATURE(LogLevel,                    E, E) \
    _ANDOR_FEATURE(MCPGain,                     X, X) \
    _ANDOR_FEATURE(MCPIntelligate,              X, X) \
    _ANDOR_FEATURE(MCPVoltage,                  X, X) \
    _ANDOR_FEATURE(MaxInterfaceTransferRate,    X, F) \
    _ANDOR_FEATURE(MetadataEnable,              X, B) \
    _ANDOR_FEATURE(MetadataFrame,               X, B) \
    _ANDOR_FEATURE(MetadataTimestamp,           X, B) \
    _ANDOR_FEATURE(MicrocodeVersion,            X, X) \
    _ANDOR_FEATURE(MultitrackBinned,            X, B) \
    _ANDOR_FEATURE(MultitrackCount,             X, I) \
    _ANDOR_FEATURE(MultitrackEnd,               X, I) \
    _ANDOR_FEATURE(MultitrackSelector,          X, I) \
    _ANDOR_FEATURE(MultitrackStart,             X, I) \
    _ANDOR_FEATURE(Overlap,                     X, B) \
    _ANDOR_FEATURE(PIVEnable,                   X, X) \
    _ANDOR_FEATURE(PixelCorrection,             E, X) \
    _ANDOR_FEATURE(PixelEncoding,               E, E) \
    _ANDOR_FEATURE(PixelHeight,                 F, F) \
    _ANDOR_FEATURE(PixelReadoutRate,            E, E) \
    _ANDOR_FEATURE(PixelWidth,                  F, F) \
    _ANDOR_FEATURE(PortSelector,                X, X) \
    _ANDOR_FEATURE(PreAmpGain,                  E, X) \
    _ANDOR_FEATURE(PreAmpGainChannel,           E, X) \
    _ANDOR_FEATURE(PreAmpGainControl,           X, X) \
    _ANDOR_FEATURE(PreAmpGainSelector,          E, X) \
    _ANDOR_FEATURE(PreAmpGainValue,             X, X) \
    _ANDOR_FEATURE(PreAmpOffsetValue,           X, X) \
    _ANDOR_FEATURE(PreTriggerEnable,            X, X) \
    _ANDOR_FEATURE(ReadoutTime,                 X, F) \
    _ANDOR_FEATURE(RollingShutterGlobalClear,   X, B) \
    _ANDOR_FEATURE(RowNExposureEndEvent,        X, I) \
    _ANDOR_FEATURE(RowNExposureStartEvent,      X, I) \
    _ANDOR_FEATURE(RowReadTime,                 X, F) \
    _ANDOR_FEATURE(ScanSpeedControlEnable,      X, B) \
    _ANDOR_FEATURE(SensorCooling,               B, B) \
    _ANDOR_FEATURE(SensorHeight,                I, I) \
    _ANDOR_FEATURE(SensorModel,                 X, X) \
    _ANDOR_FEATURE(SensorReadoutMode,           X, E) \
    _ANDOR_FEATURE(SensorTemperature,           F, F) \
    _ANDOR_FEATURE(SensorType,                  X, X) \
    _ANDOR_FEATURE(SensorWidth,                 I, I) \
    _ANDOR_FEATURE(SerialNumber,                S, S) \
    _ANDOR_FEATURE(ShutterAmpControl,           X, X) \
    _ANDOR_FEATURE(ShutterMode,                 X, E) \
    _ANDOR_FEATURE(ShutterOutputMode,           X, E) \
    _ANDOR_FEATURE(ShutterState,                X, X) \
    _ANDOR_FEATURE(ShutterStrobePeriod,         X, X) \
    _ANDOR_FEATURE(ShutterStrobePosition,       X, X) \
    _ANDOR_FEATURE(ShutterTransferTime,         X, F) \
    _ANDOR_FEATURE(SimplePreAmpGainControl,     X, E) \
    _ANDOR_FEATURE(SoftwareTrigger,             X, C) \
    _ANDOR_FEATURE(SoftwareVersion,             S, S) \
    _ANDOR_FEATURE(SpuriousNoiseFilter,         X, B) \
    _ANDOR_FEATURE(StaticBlemishCorrection,     X, B) \
    _ANDOR_FEATURE(SynchronousTriggering,       B, X) \
    _ANDOR_FEATURE(TargetSensorTemperature,     F, X) \
    _ANDOR_FEATURE(TemperatureControl,          X, E) \
    _ANDOR_FEATURE(TemperatureStatus,           X, E) \
    _ANDOR_FEATURE(TimestampClock,              X, I) \
    _ANDOR_FEATURE(TimestampClockFrequency,     X, I) \
    _ANDOR_FEATURE(TimestampClockReset,         X, C) \
    _ANDOR_FEATURE(TransmitFrames,              X, X) \
    _ANDOR_FEATURE(TriggerMode,                 E, E) \
    _ANDOR_FEATURE(UsbDeviceId,                 X, X) \
    _ANDOR_FEATURE(UsbProductId,                X, X) \
    _ANDOR_FEATURE(VerticallyCentreAOI,         X, B)

_TAO_BEGIN_DECLS

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(f,s,z) f,
typedef enum andor_feature andor_feature_t;
enum andor_feature {_ANDOR_FEATURES};

#define ANDOR_NFEATURES (1 + VerticallyCentreAOI)

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
