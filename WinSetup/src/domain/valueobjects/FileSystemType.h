// src/domain/valueobjects/FileSystemType.h
#pragma once

#include <string>

namespace winsetup::domain {

    enum class FileSystemType {
        Unknown = 0,
        NTFS = 1,
        FAT32 = 2,
        exFAT = 3,
        ReFS = 4,
        FAT = 5,
        RAW = 6
    };

    [[nodiscard]] inline std::wstring FileSystemTypeToString(FileSystemType type) noexcept {
        switch (type) {
        case FileSystemType::NTFS: return L"NTFS";
        case FileSystemType::FAT32: return L"FAT32";
        case FileSystemType::exFAT: return L"exFAT";
        case FileSystemType::ReFS: return L"ReFS";
        case FileSystemType::FAT: return L"FAT";
        case FileSystemType::RAW: return L"RAW";
        default: return L"Unknown";
        }
    }

    [[nodiscard]] inline FileSystemType ParseFileSystemType(const std::wstring& str) noexcept {
        if (str == L"NTFS") return FileSystemType::NTFS;
        if (str == L"FAT32") return FileSystemType::FAT32;
        if (str == L"exFAT") return FileSystemType::exFAT;
        if (str == L"ReFS") return FileSystemType::ReFS;
        if (str == L"FAT") return FileSystemType::FAT;
        if (str == L"RAW") return FileSystemType::RAW;
        return FileSystemType::Unknown;
    }

    [[nodiscard]] inline bool IsWindowsCompatible(FileSystemType type) noexcept {
        return type == FileSystemType::NTFS ||
            type == FileSystemType::ReFS;
    }

}
