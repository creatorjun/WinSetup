// src/application/dto/SystemAnalysisResult.h
#pragma once

#include <domain/entities/SystemInfo.h>
#include <domain/entities/DiskInfo.h>
#include <domain/entities/VolumeInfo.h>
#include <memory>
#include <vector>

namespace winsetup::application {

    /**
     * 단일 진실 공급원 (Single Source of Truth)
     *
     * 분석 완료 후 생성된 SystemAnalysisResult는 shared_ptr로만 전달됩니다.
     * 모든 UseCase는 이 객체를 const 참조로 읽기 전용 접근합니다.
     * 복사 없이 동일한 메모리를 공유하므로 데이터 불일치가 원천 차단됩니다.
     *
     * 생성: AnalyzeSystemUseCase::Execute() 에서 1회만 생성
     * 소유: MainViewModel이 shared_ptr로 보관
     * 접근: 모든 UseCase는 shared_ptr<const SystemAnalysisResult>로 수신
     */

    struct SystemAnalysisResult {
        std::shared_ptr<const domain::SystemInfo>              systemInfo;
        std::shared_ptr<const std::vector<domain::DiskInfo>>   disks;
        std::shared_ptr<const std::vector<domain::VolumeInfo>> volumes;

        SystemAnalysisResult() = default;

        SystemAnalysisResult(
            std::shared_ptr<const domain::SystemInfo>              sysInfo,
            std::shared_ptr<const std::vector<domain::DiskInfo>>   diskList,
            std::shared_ptr<const std::vector<domain::VolumeInfo>> volumeList
        )
            : systemInfo(std::move(sysInfo))
            , disks(std::move(diskList))
            , volumes(std::move(volumeList))
        {
        }

        [[nodiscard]] bool IsValid() const noexcept {
            return systemInfo != nullptr
                && disks != nullptr
                && volumes != nullptr;
        }
    };

}
