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

#undef _ANDOR_FEATURE
#define _ANDOR_FEATURE(f,s,z) f,
typedef enum andor_feature andor_feature_t;
enum andor_feature {_ANDOR_FEATURES};

#define ANDOR_NFEATURES (1 + VerticallyCentreAOI)

#endif /* _ANDOR_FEATURES_H */
