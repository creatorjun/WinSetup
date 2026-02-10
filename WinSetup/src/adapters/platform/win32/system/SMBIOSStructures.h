// src/adapters/platform/win32/system/SMBIOSStructures.h
#pragma once

#include <cstdint>

namespace winsetup::adapters::platform {

#pragma pack(push, 1)

    struct RawSMBIOSData {
        BYTE Used20CallingMethod;
        BYTE SMBIOSMajorVersion;
        BYTE SMBIOSMinorVersion;
        BYTE DmiRevision;
        DWORD Length;
        BYTE SMBIOSTableData[1];
    };

    struct SMBIOSHeader {
        BYTE Type;
        BYTE Length;
        WORD Handle;
    };

    struct SMBIOSBIOSInformation {
        SMBIOSHeader Header;
        BYTE Vendor;
        BYTE BIOSVersion;
        WORD BIOSStartingSegment;
        BYTE BIOSReleaseDate;
        BYTE BIOSROMSize;
        BYTE BIOSCharacteristics[8];
        BYTE ExtensionByte1;
        BYTE ExtensionByte2;
        BYTE SystemBIOSMajorRelease;
        BYTE SystemBIOSMinorRelease;
        BYTE EmbeddedFirmwareMajorRelease;
        BYTE EmbeddedFirmwareMinorRelease;
    };

    struct SMBIOSSystemInformation {
        SMBIOSHeader Header;
        BYTE Manufacturer;
        BYTE ProductName;
        BYTE Version;
        BYTE SerialNumber;
        BYTE UUID[16];
        BYTE WakeupType;
        BYTE SKUNumber;
        BYTE Family;
    };

    struct SMBIOSBaseboardInformation {
        SMBIOSHeader Header;
        BYTE Manufacturer;
        BYTE Product;
        BYTE Version;
        BYTE SerialNumber;
        BYTE AssetTag;
        BYTE FeatureFlags;
        BYTE LocationInChassis;
        WORD ChassisHandle;
        BYTE BoardType;
        BYTE NumberOfContainedObjectHandles;
    };

    struct SMBIOSProcessorInformation {
        SMBIOSHeader Header;
        BYTE SocketDesignation;
        BYTE ProcessorType;
        BYTE ProcessorFamily;
        BYTE ProcessorManufacturer;
        BYTE ProcessorID[8];
        BYTE ProcessorVersion;
        BYTE Voltage;
        WORD ExternalClock;
        WORD MaxSpeed;
        WORD CurrentSpeed;
        BYTE Status;
        BYTE ProcessorUpgrade;
        WORD L1CacheHandle;
        WORD L2CacheHandle;
        WORD L3CacheHandle;
        BYTE SerialNumber;
        BYTE AssetTag;
        BYTE PartNumber;
        BYTE CoreCount;
        BYTE CoreEnabled;
        BYTE ThreadCount;
        WORD ProcessorCharacteristics;
        WORD ProcessorFamily2;
    };

    struct SMBIOSMemoryDevice {
        SMBIOSHeader Header;
        WORD PhysicalMemoryArrayHandle;
        WORD MemoryErrorInformationHandle;
        WORD TotalWidth;
        WORD DataWidth;
        WORD Size;
        BYTE FormFactor;
        BYTE DeviceSet;
        BYTE DeviceLocator;
        BYTE BankLocator;
        BYTE MemoryType;
        WORD TypeDetail;
        WORD Speed;
        BYTE Manufacturer;
        BYTE SerialNumber;
        BYTE AssetTag;
        BYTE PartNumber;
        BYTE Attributes;
        DWORD ExtendedSize;
        WORD ConfiguredMemorySpeed;
        WORD MinimumVoltage;
        WORD MaximumVoltage;
        WORD ConfiguredVoltage;
    };

#pragma pack(pop)

}
