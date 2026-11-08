// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

// The test is hidden by default. Run it with the tag: Tests "[.ufbx-data]"

#include "../CommonUtils.h"
#include "UFBXDataHarness.h"

#include <Urho3D/Core/Exception.h>

#include <cstdio>

using namespace Urho3D;

namespace
{

FBXImporterSettings GetDataTestSettings()
{
    FBXImporterSettings settings;
    settings.cleanupRootNodes_ = false;
    settings.preview_.addLights_ = false;
    settings.preview_.addSkybox_ = false;
    settings.preview_.addReflectionProbe_ = false;
    settings.preview_.highRenderQuality_ = false;
    return settings;
}

enum class LoadStatus
{
    Ok,
    LoadFailed,
    ProcessFailed,
    Exception,
};

const char* ToString(LoadStatus status)
{
    switch (status)
    {
    case LoadStatus::Ok: return "load-ok";
    case LoadStatus::LoadFailed: return "load-fail";
    case LoadStatus::ProcessFailed: return "process-fail";
    case LoadStatus::Exception: return "exception";
    }
    return "unknown";
}

LoadStatus LoadAndProcess(Context* context, const UFBXData::DataPair& pair, UFBXData::ImportedGeometry& geometry,
    ea::string& error)
{
    try
    {
        UFBXData::CaptureCallback callback;
        FBXImporter importer{context, GetDataTestSettings()};
        if (!importer.LoadFile(pair.fbxPath_))
            return LoadStatus::LoadFailed;
        if (!importer.Process("", "UFBXData/", &callback))
            return LoadStatus::ProcessFailed;
        geometry = callback.geometry_;
        UFBXData::GatherImportedMeshes(context, pair.fbxPath_, callback, geometry);
        return LoadStatus::Ok;
    }
    catch (const std::exception& e)
    {
        error = e.what();
        return LoadStatus::Exception;
    }
}

} // namespace

TEST_CASE("FBX importer loads the ufbx test data", "[ufbx-data][.ufbx-data]")
{
    const UFBXData::Config config = UFBXData::ReadConfig();
    if (!config.hasData_)
    {
        WARN("Skipped: " << config.skipMessage_.c_str());
        return;
    }

    const auto pairs = UFBXData::EnumeratePairs(config.dataDir_, config.filter_);
    if (pairs.empty())
    {
        WARN("Skipped: no FBX files with OBJ baselines in " << config.dataDir_.c_str());
        return;
    }

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    unsigned numLoaded = 0;
    unsigned numSkipped = 0;
    unsigned numExpectedFailures = 0;
    unsigned numMatched = 0;
    unsigned numMismatched = 0;
    unsigned numCompareSkipped = 0;
    ea::vector<ea::string> failures;
    ea::vector<ea::string> mismatches;

    for (const UFBXData::DataPair& pair : pairs)
    {
        const UFBXData::SkipDecision skip = UFBXData::GetSkipDecision(pair);
        const char* reportName = pair.name_.c_str();

        if (skip.kind_ == UFBXData::IssueKind::Unsupported)
        {
            ++numSkipped;
            if (config.report_)
                std::printf("%s\tSKIP\t%s\n", reportName, skip.reason_.c_str());
            continue;
        }

        UFBXData::ImportedGeometry geometry;
        ea::string error;
        const LoadStatus status = LoadAndProcess(context, pair, geometry, error);

        if (status == LoadStatus::Ok)
        {
            ++numLoaded;
            // A "flag:" or "baseline:" issue makes the baseline not comparable. The test loads the file only.
            UFBXData::ComparisonResult comparison;
            if (skip.kind_ == UFBXData::IssueKind::Flag || skip.kind_ == UFBXData::IssueKind::Baseline)
                comparison = {UFBXData::ComparisonResult::Status::Skipped, skip.reason_};
            else
                comparison = UFBXData::CompareMeshes(pair, geometry);

            const char* label = "SKIP";
            switch (comparison.status_)
            {
            case UFBXData::ComparisonResult::Status::Match:
                ++numMatched;
                label = "MATCH";
                break;
            case UFBXData::ComparisonResult::Status::Mismatch:
                ++numMismatched;
                label = "MISMATCH";
                // A known issue accepts the mismatch. Do not add a real defect to the table.
                if (skip.kind_ == UFBXData::IssueKind::None)
                    mismatches.push_back(pair.name_ + ": " + comparison.message_);
                break;
            default:
                ++numCompareSkipped;
                break;
            }
            if (config.report_)
            {
                std::printf("%s\t%s\t%s%s%s\n", reportName, label, comparison.message_.c_str(),
                    skip.reason_.empty() || comparison.status_ == UFBXData::ComparisonResult::Status::Skipped
                        ? ""
                        : "; known: ",
                    comparison.status_ == UFBXData::ComparisonResult::Status::Skipped ? "" : skip.reason_.c_str());
            }
            continue;
        }

        const bool expected = skip.kind_ == UFBXData::IssueKind::UfbxExpectFail;
        if (expected)
            ++numExpectedFailures;
        else
            failures.push_back(pair.name_);

        if (config.report_)
        {
            std::printf("%s\t%s\t%s%s%s\n", reportName, ToString(status),
                expected ? skip.reason_.c_str() : "unexpected", error.empty() ? "" : "; ", error.c_str());
        }
    }

    if (config.report_)
    {
        std::printf("ufbx-data: pairs=%u loaded=%u skipped=%u expected-failures=%u unexpected-failures=%u\n",
            (unsigned)pairs.size(), numLoaded, numSkipped, numExpectedFailures, (unsigned)failures.size());
        std::printf("ufbx-data: match=%u mismatch=%u compare-skipped=%u\n", numMatched, numMismatched,
            numCompareSkipped);
        std::fflush(stdout);
    }

    for (const ea::string& name : failures)
        FAIL_CHECK("Unexpected import failure: " << name.c_str());
    for (const ea::string& message : mismatches)
        FAIL_CHECK("Geometry mismatch: " << message.c_str());
}
