// src/domain/valueobjects/FileSystemType.h

#pragma once

#include <string>

namespace winsetup::domain {

    enum class FileSystemType {
        Unknown,
        NTFS,
        FAT32,
        FAT16,
        FAT12,
        exFAT,
        ReFS,
        EXT2,
        EXT3,
        EXT4,
        RAW
    };

    inline std::wstring FileSystemTypeToString(FileSystemType fs) {
        switch (fs) {
        case FileSystemType::NTFS:   return L"NTFS";
        case FileSystemType::FAT32:  return L"FAT32";
        case FileSystemType::FAT16:  return L"FAT16";
        case FileSystemType::FAT12:  return L"FAT12";
        case FileSystemType::exFAT:  return L"exFAT";
        case FileSystemType::ReFS:   return L"ReFS";
        case FileSystemType::EXT2:   return L"EXT2";
        case FileSystemType::EXT3:   return L"EXT3";
        case FileSystemType::EXT4:   return L"EXT4";
        case FileSystemType::RAW:    return L"RAW";
        default:                     return L"Unknown";
        }
    }

    inline FileSystemType ParseFileSystemType(const std::wstring& fsStr) {
        std::wstring upper = fsStr;
        for (auto& c : upper) {
            c = static_cast<wchar_t>(::towupper(c));
        }

        if (upper == L"NTFS") return FileSystemType::NTFS;
        if (upper == L"FAT32") return FileSystemType::FAT32;
        if (upper == L"FAT16") return FileSystemType::FAT16;
        if (upper == L"FAT12") return FileSystemType::FAT12;
        if (upper == L"EXFAT") return FileSystemType::exFAT;
        if (upper == L"REFS") return FileSystemType::ReFS;
        if (upper == L"EXT2") return FileSystemType::EXT2;
        if (upper == L"EXT3") return FileSystemType::EXT3;
        if (upper == L"EXT4") return FileSystemType::EXT4;
        if (upper == L"RAW") return FileSystemType::RAW;

        return FileSystemType::Unknown;
    }

    inline bool IsWindowsFileSystem(FileSystemType fs) noexcept {
        return fs == FileSystemType::NTFS ||
            fs == FileSystemType::FAT32 ||
            fs == FileSystemType::FAT16 ||
            fs == FileSystemType::exFAT ||
            fs == FileSystemType::ReFS;
    }

}
