// src/adapters/platform/win32/system/SMBIOSStructures.h
#pragma once

#include <Windows.h>
#include <cstdint>

#pragma pack(push, 1)

namespace winsetup::adapters::platform {

    struct RawSMBIOSData {
        BYTE    Used20CallingMethod;
        BYTE    SMBIOSMajorVersion;
        BYTE    SMBIOSMinorVersion;
        BYTE    DmiRevision;
        DWORD   Length;
        BYTE    SMBIOSTableData[1];
    };

    struct SMBIOSHeader {
        BYTE    Type;
        BYTE    Length;
        WORD    Handle;
    };

    struct SMBIOSBIOSInformation {
        BYTE    Type;
        BYTE    Length;
        WORD    Handle;
        BYTE    Vendor;
        BYTE    BIOSVersion;
        WORD    BIOSStartingSegment;
        BYTE    BIOSReleaseDate;
        BYTE    BIOSROMSize;
        DWORD   BIOSCharacteristics[2];
        BYTE    ExtensionByte1;
        BYTE    ExtensionByte2;
        BYTE    SystemBIOSMajorRelease;
        BYTE    SystemBIOSMinorRelease;
        BYTE    EmbeddedFirmwareMajorRelease;
        BYTE    EmbeddedFirmwareMinorRelease;
    };

    struct SMBIOSSystemInformation {
        BYTE    Type;
        BYTE    Length;
        WORD    Handle;
        BYTE    Manufacturer;
        BYTE    ProductName;
        BYTE    Version;
        BYTE    SerialNumber;
        BYTE    UUID[16];
        BYTE    WakeupType;
        BYTE    SKUNumber;
        BYTE    Family;
    };

    struct SMBIOSBaseboardInformation {
        BYTE    Type;
        BYTE    Length;
        WORD    Handle;
        BYTE    Manufacturer;
        BYTE    Product;
        BYTE    Version;
        BYTE    SerialNumber;
        BYTE    AssetTag;
        BYTE    FeatureFlags;
        BYTE    LocationInChassis;
        WORD    ChassisHandle;
        BYTE    BoardType;
        BYTE    NumberOfContainedObjectHandles;
    };

    struct SMBIOSProcessorInformation {
        BYTE    Type;
        BYTE    Length;
        WORD    Handle;
        BYTE    SocketDesignation;
        BYTE    ProcessorType;
        BYTE    ProcessorFamily;
        BYTE    ProcessorManufacturer;
        DWORD   ProcessorID[2];
        BYTE    ProcessorVersion;
        BYTE    Voltage;
        WORD    ExternalClock;
        WORD    MaxSpeed;
        WORD    CurrentSpeed;
        BYTE    Status;
        BYTE    ProcessorUpgrade;
        WORD    L1CacheHandle;
        WORD    L2CacheHandle;
        WORD    L3CacheHandle;
        BYTE    SerialNumber;
        BYTE    AssetTag;
        BYTE    PartNumber;
        BYTE    CoreCount;
        BYTE    CoreEnabled;
        BYTE    ThreadCount;
        WORD    ProcessorCharacteristics;
        WORD    ProcessorFamily2;
    };

    struct SMBIOSMemoryDevice {
        BYTE    Type;
        BYTE    Length;
        WORD    Handle;
        WORD    PhysicalMemoryArrayHandle;
        WORD    MemoryErrorInformationHandle;
        WORD    TotalWidth;
        WORD    DataWidth;
        WORD    Size;
        BYTE    FormFactor;
        BYTE    DeviceSet;
        BYTE    DeviceLocator;
        BYTE    BankLocator;
        BYTE    MemoryType;
        WORD    TypeDetail;
        WORD    Speed;
        BYTE    Manufacturer;
        BYTE    SerialNumber;
        BYTE    AssetTag;
        BYTE    PartNumber;
        BYTE    Attributes;
        DWORD   ExtendedSize;
        WORD    ConfiguredMemorySpeed;
        WORD    MinimumVoltage;
        WORD    MaximumVoltage;
        WORD    ConfiguredVoltage;
    };

}

#pragma pack(pop)
