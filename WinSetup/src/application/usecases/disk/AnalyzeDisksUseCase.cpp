// src/application/usecases/disk/AnalyzeDisksUseCase.cpp
#include "application/usecases/disk/AnalyzeDisksUseCase.h"
#include "domain/valueobjects/DiskType.h"
#include <algorithm>

namespace winsetup::application {

    AnalyzeDisksUseCase::AnalyzeDisksUseCase(
        std::shared_ptr<abstractions::IAnalysisRepository> analysisRepository,
        std::shared_ptr<abstractions::ILogger>             logger
    )
        : mAnalysisRepository(std::move(analysisRepository))
        , mLogger(std::move(logger))
    {
    }

    domain::Expected<void> AnalyzeDisksUseCase::Execute()
    {
        if (!mAnalysisRepository)
            return domain::Error(L"IAnalysisRepository not provided", 0, domain::ErrorCategory::System);

        if (mLogger)
            mLogger->Info(L"AnalyzeDisksUseCase: Started.");

        auto diskResult = mAnalysisRepository->GetDisks();
        if (!diskResult.HasValue())
            return diskResult.GetError();

        std::vector<domain::DiskInfo> disks(*diskResult.Value());

        if (AllRolesAssigned(*mAnalysisRepository)) {
            if (mLogger)
                mLogger->Info(L"AnalyzeDisksUseCase: All volume roles assigned. Assigning disks by volume ownership.");
            AssignByVolumes(disks, *mAnalysisRepository);
        }
        else {
            if (mLogger)
                mLogger->Warning(L"AnalyzeDisksUseCase: One or more volume roles missing. Assigning disks by interface priority.");
            AssignByPriority(disks);
        }

        bool foundSystem = false, foundData = false;
        for (const auto& disk : disks) {
            if (disk.IsSystem()) {
                if (mLogger)
                    mLogger->Info(L"AnalyzeDisksUseCase: [System] Disk "
                        + std::to_wstring(disk.GetIndex())
                        + L" [" + disk.GetModel() + L"] "
                        + std::to_wstring(static_cast<uint64_t>(disk.GetSize().ToGB())) + L" GB");
                foundSystem = true;
            }
            if (disk.IsData()) {
                if (mLogger)
                    mLogger->Info(L"AnalyzeDisksUseCase: [Data]   Disk "
                        + std::to_wstring(disk.GetIndex())
                        + L" [" + disk.GetModel() + L"] "
                        + std::to_wstring(static_cast<uint64_t>(disk.GetSize().ToGB())) + L" GB");
                foundData = true;
            }
        }

        if (!foundSystem && mLogger)
            mLogger->Warning(L"AnalyzeDisksUseCase: [System] disk not found");
        if (!foundData && mLogger)
            mLogger->Warning(L"AnalyzeDisksUseCase: [Data]   disk not found");

        mAnalysisRepository->StoreUpdatedDisks(std::move(disks));

        if (mLogger)
            mLogger->Info(L"AnalyzeDisksUseCase: Complete.");

        return domain::Expected<void>();
    }

    bool AnalyzeDisksUseCase::AllRolesAssigned(const abstractions::IAnalysisRepository& repo)
    {
        return repo.GetSystemVolume().has_value()
            && repo.GetDataVolume().has_value()
            && repo.GetBootVolume().has_value();
    }

    domain::DiskInfo* AnalyzeDisksUseCase::FindDiskByIndex(
        std::vector<domain::DiskInfo>& disks, uint32_t index)
    {
        auto it = std::find_if(disks.begin(), disks.end(),
            [index](const domain::DiskInfo& d) { return d.GetIndex() == index; });
        return it != disks.end() ? &(*it) : nullptr;
    }

    void AnalyzeDisksUseCase::AssignByVolumes(
        std::vector<domain::DiskInfo>& disks,
        const abstractions::IAnalysisRepository& repo)
    {
        const auto sysDiskOpt = repo.GetSystemDisk();
        const auto dataDiskOpt = repo.GetDataDisk();

        if (sysDiskOpt.has_value()) {
            if (auto* disk = FindDiskByIndex(disks, sysDiskOpt->GetIndex()))
                disk->SetIsSystem(true);
        }

        if (dataDiskOpt.has_value()) {
            if (auto* disk = FindDiskByIndex(disks, dataDiskOpt->GetIndex()))
                disk->SetIsData(true);
        }
    }

    void AnalyzeDisksUseCase::AssignByPriority(
        std::vector<domain::DiskInfo>& disks)
    {
        std::vector<domain::DiskInfo*> candidates;
        candidates.reserve(disks.size());
        for (auto& disk : disks)
            candidates.push_back(&disk);

        std::stable_sort(candidates.begin(), candidates.end(),
            [](const domain::DiskInfo* a, const domain::DiskInfo* b) {
                const int pa = domain::GetDiskTypePriority(a->GetDiskType());
                const int pb = domain::GetDiskTypePriority(b->GetDiskType());
                if (pa != pb)
                    return pa < pb;
                return a->GetSize().ToBytes() < b->GetSize().ToBytes();
            });

        if (candidates.size() >= 1)
            candidates[0]->SetIsSystem(true);

        if (candidates.size() >= 2)
            candidates[1]->SetIsData(true);
        else if (candidates.size() == 1)
            candidates[0]->SetIsData(true);
    }

}
