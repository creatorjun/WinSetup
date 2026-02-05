#include <gtest/gtest.h>
#include "../../src/application/usecases/DiskAnalyzer.h"
#include "../../src/adapters/platform/windows/storage/Win32DiskService.h"
#include "../../src/adapters/platform/windows/storage/Win32VolumeService.h"
#include "../../src/adapters/platform/windows/storage/Win32StorageScanner.h"
#include "../../src/adapters/platform/windows/encoding/Win32TextEncoder.h"

using namespace winsetup;

class DiskAnalysisIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        textEncoder_ = adapters::CreateTextEncoder();
        diskService_ = adapters::CreateDiskService(textEncoder_);
        volumeService_ = adapters::CreateVolumeService(textEncoder_);
        storageScanner_ = adapters::CreateStorageScanner(volumeService_, textEncoder_);

        analyzer_ = std::make_unique<application::DiskAnalyzer>(
            diskService_,
            volumeService_,
            storageScanner_
        );
    }

    std::shared_ptr<abstractions::ITextEncoder> textEncoder_;
    std::shared_ptr<abstractions::IDiskService> diskService_;
    std::shared_ptr<abstractions::IVolumeService> volumeService_;
    std::shared_ptr<abstractions::IStorageScanner> storageScanner_;
    std::unique_ptr<application::DiskAnalyzer> analyzer_;
};

TEST_F(DiskAnalysisIntegrationTest, EnumerateAndSortDisks) {
    auto result = analyzer_->GetSortedDisks();

    ASSERT_TRUE(result.HasValue());

    const auto& disks = result.Value();
    EXPECT_FALSE(disks.empty());

    for (const auto& disk : disks) {
        std::wcout << L"Disk " << disk.GetId().index
            << L": " << domain::BusTypeToString(disk.GetBusType())
            << L" - " << disk.GetSizeInGB() << L" GB"
            << L" - " << disk.GetModel()
            << std::endl;
    }

    if (disks.size() >= 2) {
        EXPECT_GE(
            disks[0].GetPriority(),
            disks[1].GetPriority()
        );
    }
}

TEST_F(DiskAnalysisIntegrationTest, FindSystemVolume) {
    auto result = analyzer_->FindSystemVolume();

    if (result.HasValue()) {
        const auto& volume = result.Value();

        std::wcout << L"System Volume: " << volume.GetDriveLetterString()
            << L" (" << volume.GetLabel() << L")"
            << L" - " << domain::FileSystemTypeToString(volume.GetFileSystemType())
            << std::endl;

        EXPECT_TRUE(volume.HasDriveLetter());
        EXPECT_TRUE(volume.IsNTFS() ||
            volume.GetFileSystemType() == domain::FileSystemType::ReFS);
    }
}

TEST_F(DiskAnalysisIntegrationTest, FullSystemAnalysis) {
    auto result = analyzer_->AnalyzeSystem(L"testuser");

    ASSERT_TRUE(result.HasValue());

    const auto& analysis = result.Value();

    EXPECT_FALSE(analysis.sortedDisks.empty());
    EXPECT_GT(analysis.primaryDisk.GetSizeInBytes(), 0);

    std::wcout << L"\n=== System Analysis Result ===" << std::endl;
    std::wcout << L"Primary Disk: " << analysis.primaryDisk.GetModel() << std::endl;
    std::wcout << L"Has Existing Windows: "
        << (analysis.hasExistingWindows ? L"Yes" : L"No") << std::endl;
    std::wcout << L"Has User Data: "
        << (analysis.hasUserData ? L"Yes" : L"No") << std::endl;
}

TEST_F(DiskAnalysisIntegrationTest, VolumeScanning) {
    auto volumesResult = volumeService_->EnumerateVolumes();
    ASSERT_TRUE(volumesResult.HasValue());

    const auto& volumes = volumesResult.Value();

    for (const auto& volume : volumes) {
        if (!volume.HasDriveLetter()) continue;

        auto scanResult = storageScanner_->ScanVolume(volume, L"testuser");
        if (scanResult.HasError()) continue;

        const auto& scan = scanResult.Value();

        std::wcout << L"\nVolume: " << volume.GetDriveLetterString() << std::endl;
        std::wcout << L"  Score: " << scan.score << std::endl;
        std::wcout << L"  Has Windows: " << scan.hasWindowsDirectory << std::endl;
        std::wcout << L"  Has System32: " << scan.hasSystem32Directory << std::endl;
        std::wcout << L"  Likely System: " << scan.IsLikelySystemVolume() << std::endl;
        std::wcout << L"  Likely Data: " << scan.IsLikelyDataVolume() << std::endl;
    }
}
