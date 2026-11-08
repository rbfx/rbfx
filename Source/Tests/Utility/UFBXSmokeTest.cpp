// Copyright (c) 2026 the rbfx project.
// This work is licensed under the terms of the MIT license.

// Smoke and stress tests for FBXImporter with all FBX files of the ufbx test data.
// The tests are hidden by default. Run them with the tags:
//   Tests "[.ufbx-smoke]"   Import all FBX files, check invariants of the result, check determinism.
//   Tests "[.ufbx-stress]"  Import truncated copies of 10 small FBX files.
// Environment variables:
//   UFBX_DATA_DIR      Path to the "data" directory. The tests skip when it is not set or missing.
//   UFBX_DATA_FILTER   Substring. The smoke test uses only the files with this substring in the relative path.
//   UFBX_DATA_REPORT   Set to 1 to print one line for each file: name, status and reason.
//   UFBX_SMOKE_SKIP    Comma-separated substrings. The smoke test skips the files that contain one of them.
//                      Use it to continue after a crash.

#include "../CommonUtils.h"
#include "UFBXDataHarness.h"

#include <Urho3D/Core/Exception.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/ModelView.h>
#include <Urho3D/IO/IOEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Serializable.h>

#include <EASTL/algorithm.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace Urho3D;

namespace
{

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

/// A file takes more time than this limit: the test reports a hang and fails.
constexpr double TimeLimitSeconds = 30.0;

FBXImporterSettings GetSmokeSettings()
{
    FBXImporterSettings settings;
    settings.cleanupRootNodes_ = false;
    settings.preview_.addLights_ = false;
    settings.preview_.addSkybox_ = false;
    settings.preview_.addReflectionProbe_ = false;
    settings.preview_.highRenderQuality_ = false;
    return settings;
}

ea::string ToEa(const std::string& value)
{
    return ea::string(value.c_str(), value.size());
}

bool Contains(const ea::string& text, const char* part)
{
    return text.find(part) != ea::string::npos;
}

/// Files that must fail to load. The test does not report an error for them.
bool IsExpectedError(const ea::string& name)
{
    // synthetic_id_collision has a node chain of depth 10000. The importer limits the node depth to 1024.
    return Contains(name, "_fail") || Contains(name, "casegen_bad_path") || Contains(name, "truncated")
        || Contains(name, "synthetic_id_collision");
}

/// Hash of a byte range (FNV-1a).
class Hasher
{
public:
    void AddBytes(const void* data, size_t size)
    {
        const auto* bytes = static_cast<const unsigned char*>(data);
        for (size_t i = 0; i < size; ++i)
        {
            hash_ ^= bytes[i];
            hash_ *= 1099511628211ull;
        }
    }
    template <class T> void Add(const T& value) { AddBytes(&value, sizeof(T)); }
    uint64_t Get() const { return hash_; }

private:
    uint64_t hash_{14695981039346656037ull};
};

/// Importer callback that checks the invariants of the imported data and hashes the geometry.
class SmokeCallback : public FBXImporterCallback
{
public:
    /// First invariant violation. Empty if there is none.
    ea::string violation_;
    unsigned numModels_{};
    unsigned numAnimations_{};

    uint64_t GetGeometryHash() const { return hasher_.Get(); }

    void OnModelLoaded(ModelView& modelView) override
    {
        ++numModels_;
        const unsigned numBones = modelView.GetBones().size();
        hasher_.Add(numBones);

        unsigned geometryIndex = 0;
        for (GeometryView& geometry : modelView.GetGeometries())
        {
            for (unsigned lodIndex = 0; lodIndex < geometry.lods_.size(); ++lodIndex)
            {
                GeometryLODView& lod = geometry.lods_[lodIndex];
                CheckLod(modelView.GetName(), geometryIndex, lodIndex, lod, numBones);
                HashLod(lod);
            }
            ++geometryIndex;
        }
    }

    void OnAnimationLoaded(Animation& animation) override
    {
        ++numAnimations_;
        const ea::string& name = animation.GetAnimationName();
        hasher_.Add(animation.GetNumTracks());
        if (!std::isfinite(animation.GetLength()))
            Fail(Format("animation '{}': length is not finite", name));

        for (const auto& [hash, track] : animation.GetTracks())
        {
            for (unsigned i = 0; i < track.keyFrames_.size(); ++i)
            {
                const AnimationKeyFrame& key = track.keyFrames_[i];
                if (!std::isfinite(key.time_))
                    Fail(Format("animation '{}' track '{}' key {}: time is not finite", name, track.name_, i));
                if (!IsFinite(key.position_) || !IsFinite(key.scale_)
                    || !IsFinite(Vector4(key.rotation_.w_, key.rotation_.x_, key.rotation_.y_, key.rotation_.z_)))
                {
                    Fail(Format("animation '{}' track '{}' key {}: value is not finite", name, track.name_, i));
                }
            }
        }
    }

private:
    static bool IsFinite(const Vector3& v) { return std::isfinite(v.x_) && std::isfinite(v.y_) && std::isfinite(v.z_); }
    static bool IsFinite(const Vector4& v)
    {
        return std::isfinite(v.x_) && std::isfinite(v.y_) && std::isfinite(v.z_) && std::isfinite(v.w_);
    }

    void Fail(const ea::string& message)
    {
        if (violation_.empty())
            violation_ = message;
    }

    void CheckLod(const ea::string& modelName, unsigned geometryIndex, unsigned lodIndex, const GeometryLODView& lod,
        unsigned numBones)
    {
        if (!violation_.empty())
            return;
        const unsigned numVertices = lod.vertices_.size();
        const unsigned numIndices = lod.indices_.size();
        const bool hasSkin = lod.vertexFormat_.blendIndices_ != ModelVertexFormat::Undefined;
        const bool hasNormals = lod.vertexFormat_.normal_ != ModelVertexFormat::Undefined;

        for (unsigned i = 0; i < numVertices; ++i)
        {
            const ModelVertex& vertex = lod.vertices_[i];
            const char* what = nullptr;
            if (!IsFinite(vertex.position_))
                what = "position";
            else if (hasNormals && !IsFinite(vertex.normal_))
                what = "normal";
            else if (!IsFinite(vertex.uv_[0]) || !IsFinite(vertex.uv_[1]))
                what = "uv";
            if (what)
            {
                Fail(Format("model '{}' geometry {} lod {} vertex {}: {} is not finite", modelName, geometryIndex,
                    lodIndex, i, what));
                return;
            }

            if (hasSkin)
            {
                const float indices[4] = {
                    vertex.blendIndices_.x_, vertex.blendIndices_.y_, vertex.blendIndices_.z_, vertex.blendIndices_.w_};
                const float weights[4] = {
                    vertex.blendWeights_.x_, vertex.blendWeights_.y_, vertex.blendWeights_.z_, vertex.blendWeights_.w_};
                for (unsigned j = 0; j < 4; ++j)
                {
                    if (!std::isfinite(weights[j]) || weights[j] < 0.0f)
                    {
                        Fail(Format("model '{}' geometry {} lod {} vertex {}: blend weight {} is invalid ({})",
                            modelName, geometryIndex, lodIndex, i, j, weights[j]));
                        return;
                    }
                    if (!std::isfinite(indices[j]) || indices[j] < 0.0f || indices[j] >= static_cast<float>(numBones))
                    {
                        Fail(Format("model '{}' geometry {} lod {} vertex {}: blend index {} is out of range ({}, {} bones)",
                            modelName, geometryIndex, lodIndex, i, j, indices[j], numBones));
                        return;
                    }
                }
            }
        }

        for (unsigned i = 0; i < numIndices; ++i)
        {
            if (lod.indices_[i] >= numVertices)
            {
                Fail(Format("model '{}' geometry {} lod {}: index {} is {}, vertex count is {}", modelName,
                    geometryIndex, lodIndex, i, lod.indices_[i], numVertices));
                return;
            }
        }
        if (lod.primitiveType_ == TRIANGLE_LIST && numIndices % 3 != 0)
        {
            Fail(Format("model '{}' geometry {} lod {}: index count {} is not a multiple of 3", modelName,
                geometryIndex, lodIndex, numIndices));
            return;
        }

        for (const auto& [morphIndex, morphVertices] : lod.morphs_)
        {
            for (const ModelVertexMorph& morph : morphVertices)
            {
                if (morph.index_ >= numVertices || !IsFinite(morph.positionDelta_) || !IsFinite(morph.normalDelta_))
                {
                    Fail(Format("model '{}' geometry {} lod {} morph {}: invalid vertex {}", modelName, geometryIndex,
                        lodIndex, morphIndex, morph.index_));
                    return;
                }
            }
        }
    }

    void HashLod(const GeometryLODView& lod)
    {
        hasher_.Add(static_cast<unsigned>(lod.primitiveType_));
        hasher_.Add(static_cast<unsigned>(lod.vertices_.size()));
        for (const ModelVertex& vertex : lod.vertices_)
        {
            hasher_.Add(vertex.position_);
            hasher_.Add(vertex.normal_);
            hasher_.Add(vertex.uv_[0]);
            hasher_.Add(vertex.blendIndices_);
            hasher_.Add(vertex.blendWeights_);
        }
        hasher_.Add(static_cast<unsigned>(lod.indices_.size()));
        if (!lod.indices_.empty())
            hasher_.AddBytes(lod.indices_.data(), lod.indices_.size() * sizeof(unsigned));
    }

    Hasher hasher_;
};

enum class ImportStatus
{
    Ok,
    Error,
    Exception,
};

struct ImportResult
{
    ImportStatus status_{ImportStatus::Ok};
    /// Error message for Error and Exception.
    ea::string message_;
    ea::string violation_;
    uint64_t hash_{};
    unsigned numModels_{};
    unsigned numAnimations_{};
    double seconds_{};
};

/// Import a file or a memory buffer. Catch all exceptions and keep the last error message from the log.
ImportResult ImportOnce(Context* context, const ea::string& path, const ea::vector<unsigned char>* memory)
{
    ImportResult result;
    ea::string lastError;
    Serializable subscriber(context);
    subscriber.SubscribeToEvent(E_LOGMESSAGE,
        [&](StringHash, VariantMap& data)
        {
            using namespace LogMessage;
            if (data[P_LEVEL].GetInt() == LOG_ERROR)
                lastError = data[P_MESSAGE].GetString();
        });

    const auto start = Clock::now();
    try
    {
        SmokeCallback callback;
        {
            FBXImporter importer{context, GetSmokeSettings()};
            const bool loaded = memory ? importer.LoadFileBinary(ConstByteSpan(memory->data(), memory->size()))
                                       : importer.LoadFile(path);
            if (!loaded)
            {
                result.status_ = ImportStatus::Error;
                result.message_ = lastError.empty() ? ea::string("load failed") : lastError;
            }
            else if (!importer.Process("", "UFBXSmoke/", &callback))
            {
                result.status_ = ImportStatus::Error;
                result.message_ = lastError.empty() ? ea::string("process failed") : lastError;
            }
        }
        result.violation_ = callback.violation_;
        result.hash_ = callback.GetGeometryHash();
        result.numModels_ = callback.numModels_;
        result.numAnimations_ = callback.numAnimations_;
    }
    catch (const std::exception& e)
    {
        result.status_ = ImportStatus::Exception;
        result.message_ = e.what();
    }
    result.seconds_ = std::chrono::duration<double>(Clock::now() - start).count();

    // Keep the message on one short line.
    for (char& c : result.message_)
    {
        if (c == '\n' || c == '\r' || c == '\t')
            c = ' ';
    }
    if (result.message_.size() > 200)
        result.message_.resize(200);
    return result;
}

/// Thread that prints a message when one import takes more time than the limit. It does not stop the import.
class Watchdog
{
public:
    Watchdog() { thread_ = std::thread([this] { Run(); }); }

    ~Watchdog()
    {
        stop_ = true;
        thread_.join();
    }

    void Begin(const ea::string& name)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        name_ = std::string(name.c_str(), name.size());
        start_ = Clock::now();
        active_ = true;
        reported_ = false;
    }

    void End()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        active_ = false;
    }

private:
    void Run()
    {
        while (!stop_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::lock_guard<std::mutex> lock(mutex_);
            if (active_ && !reported_ && std::chrono::duration<double>(Clock::now() - start_).count() > TimeLimitSeconds)
            {
                reported_ = true;
                std::fprintf(stderr, "ufbx-smoke: HANG? '%s' runs for more than %.0f s\n", name_.c_str(),
                    TimeLimitSeconds);
                std::fflush(stderr);
            }
        }
    }

    std::thread thread_;
    std::atomic<bool> stop_{false};
    std::mutex mutex_;
    std::string name_;
    Clock::time_point start_;
    bool active_{};
    bool reported_{};
};

struct SmokeFile
{
    /// Path relative to the data directory, with "/" as separator.
    ea::string name_;
    ea::string path_;
    bool isTopLevel_{};
    bool isFuzz_{};
};

std::string ToLowerExtension(const fs::path& path)
{
    std::string ext = path.extension().string();
    for (char& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

/// Find all FBX files in the data directory, recursively. The result is sorted by relative path.
ea::vector<SmokeFile> EnumerateAllFbx(const ea::string& dataDir, const ea::string& filter)
{
    ea::vector<SmokeFile> files;
    std::error_code error;
    const fs::path root(dataDir.c_str());
    for (fs::recursive_directory_iterator it(root, error), end; !error && it != end; it.increment(error))
    {
        if (!it->is_regular_file(error) || ToLowerExtension(it->path()) != ".fbx")
            continue;
        SmokeFile file;
        file.name_ = ToEa(it->path().lexically_relative(root).generic_string());
        file.path_ = ToEa(it->path().string());
        file.isTopLevel_ = !Contains(file.name_, "/");
        file.isFuzz_ = file.name_.starts_with("fuzz/");
        if (!filter.empty() && !Contains(file.name_, filter.c_str()))
            continue;
        files.push_back(file);
    }
    ea::sort(files.begin(), files.end(), [](const SmokeFile& a, const SmokeFile& b) { return a.name_ < b.name_; });
    return files;
}

bool IsSkipped(const ea::string& name)
{
    const char* skip = std::getenv("UFBX_SMOKE_SKIP");
    if (!skip)
        return false;
    std::string list(skip);
    size_t pos = 0;
    while (pos <= list.size())
    {
        size_t next = list.find(',', pos);
        if (next == std::string::npos)
            next = list.size();
        const std::string item = list.substr(pos, next - pos);
        if (!item.empty() && Contains(name, item.c_str()))
            return true;
        pos = next + 1;
    }
    return false;
}

} // namespace

TEST_CASE("FBX importer survives all ufbx test files", "[ufbx-smoke][.ufbx-smoke]")
{
    const UFBXData::Config config = UFBXData::ReadConfig();
    if (!config.hasData_)
    {
        WARN("Skipped: " << config.skipMessage_.c_str());
        return;
    }

    const auto files = EnumerateAllFbx(config.dataDir_, config.filter_);
    if (files.empty())
    {
        WARN("Skipped: no FBX files in " << config.dataDir_.c_str());
        return;
    }

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    unsigned numOk = 0;
    unsigned numCleanError = 0;
    unsigned numExpectedError = 0;
    unsigned numUnexpectedError = 0;
    unsigned numExceptions = 0;
    unsigned numViolations = 0;
    unsigned numNondeterministic = 0;
    unsigned numSlow = 0;
    unsigned numSkipped = 0;
    unsigned numDeterminismChecked = 0;
    ea::vector<ea::string> failures;
    ea::vector<ea::string> unexpectedErrors;

    const auto testStart = Clock::now();
    Watchdog watchdog;
    for (const SmokeFile& file : files)
    {
        const char* reportName = file.name_.c_str();
        if (IsSkipped(file.name_))
        {
            ++numSkipped;
            if (config.report_)
                std::printf("%s\tSKIP\tUFBX_SMOKE_SKIP\n", reportName);
            continue;
        }

        // Print the name first. If the process crashes, the last name is the crashing file.
        if (config.report_)
        {
            std::fprintf(stderr, "ufbx-smoke: begin %s\n", reportName);
            std::fflush(stderr);
        }

        watchdog.Begin(file.name_);
        const ImportResult first = ImportOnce(context, file.path_, nullptr);
        watchdog.End();

        const char* label = "OK";
        ea::string detail;
        switch (first.status_)
        {
        case ImportStatus::Ok: ++numOk; break;
        case ImportStatus::Error:
            ++numCleanError;
            label = "ERROR";
            detail = "error: " + first.message_;
            if (IsExpectedError(file.name_))
                ++numExpectedError;
            else if (file.isFuzz_)
                ; // Fuzz files are allowed to fail.
            else
            {
                ++numUnexpectedError;
                unexpectedErrors.push_back(file.name_ + ": " + first.message_);
            }
            break;
        case ImportStatus::Exception:
            ++numExceptions;
            label = "EXCEPTION";
            detail = first.message_;
            failures.push_back(file.name_ + ": exception escaped: " + first.message_);
            break;
        }

        if (!first.violation_.empty())
        {
            ++numViolations;
            label = "VIOLATION";
            detail = first.violation_;
            failures.push_back(file.name_ + ": " + first.violation_);
        }

        if (first.seconds_ > TimeLimitSeconds)
        {
            ++numSlow;
            failures.push_back(file.name_ + Format(": import took {:.1f} s, the limit is {:.0f} s", first.seconds_,
                                                TimeLimitSeconds));
        }

        // Determinism: import the file again and compare the hash. Only the top-level files.
        if (file.isTopLevel_ && first.status_ == ImportStatus::Ok && first.violation_.empty())
        {
            ++numDeterminismChecked;
            watchdog.Begin(file.name_);
            const ImportResult second = ImportOnce(context, file.path_, nullptr);
            watchdog.End();
            if (second.status_ != ImportStatus::Ok || second.hash_ != first.hash_)
            {
                ++numNondeterministic;
                label = "NONDETERMINISTIC";
                detail = second.status_ != ImportStatus::Ok ? "second import failed: " + second.message_
                                                            : ea::string("geometry hash differs");
                failures.push_back(file.name_ + ": " + detail);
            }
        }

        if (config.report_)
        {
            std::printf("%s\t%s\t%s\t%.2fs\tmodels=%u animations=%u\n", reportName, label, detail.c_str(),
                first.seconds_, first.numModels_, first.numAnimations_);
            std::fflush(stdout);
        }
    }

    const double total = std::chrono::duration<double>(Clock::now() - testStart).count();
    if (config.report_)
    {
        std::printf("ufbx-smoke: files=%u ok=%u clean-error=%u (expected=%u unexpected=%u) exception=%u "
                    "invariant-violation=%u nondeterministic=%u/%u slow=%u skipped=%u time=%.1fs\n",
            (unsigned)files.size(), numOk, numCleanError, numExpectedError, numUnexpectedError, numExceptions,
            numViolations, numNondeterministic, numDeterminismChecked, numSlow, numSkipped, total);
        for (const ea::string& message : unexpectedErrors)
            std::printf("ufbx-smoke: unexpected error: %s\n", message.c_str());
        std::fflush(stdout);
    }

    for (const ea::string& message : failures)
        FAIL_CHECK(message.c_str());
}

TEST_CASE("FBX importer survives truncated ufbx test files", "[ufbx-stress][.ufbx-stress]")
{
    const UFBXData::Config config = UFBXData::ReadConfig();
    if (!config.hasData_)
    {
        WARN("Skipped: " << config.skipMessage_.c_str());
        return;
    }

    // Choose the 5 smallest binary and the 5 smallest ASCII top-level files.
    struct Candidate
    {
        ea::string name_;
        ea::string path_;
        uintmax_t size_{};
    };
    ea::vector<Candidate> binary;
    ea::vector<Candidate> ascii;
    for (const SmokeFile& file : EnumerateAllFbx(config.dataDir_, ""))
    {
        if (!file.isTopLevel_ || IsExpectedError(file.name_))
            continue;
        std::error_code error;
        const uintmax_t size = fs::file_size(fs::path(file.path_.c_str()), error);
        if (error || size == 0)
            continue;
        if (Contains(file.name_, "_binary.fbx"))
            binary.push_back({file.name_, file.path_, size});
        else if (Contains(file.name_, "_ascii.fbx"))
            ascii.push_back({file.name_, file.path_, size});
    }
    const auto bySize = [](const Candidate& a, const Candidate& b) { return a.size_ < b.size_; };
    ea::sort(binary.begin(), binary.end(), bySize);
    ea::sort(ascii.begin(), ascii.end(), bySize);
    if (binary.size() > 5)
        binary.resize(5);
    if (ascii.size() > 5)
        ascii.resize(5);
    binary.insert(binary.end(), ascii.begin(), ascii.end());
    if (binary.empty())
    {
        WARN("Skipped: no FBX files in " << config.dataDir_.c_str());
        return;
    }

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    unsigned numImports = 0;
    unsigned numOk = 0;
    unsigned numCleanError = 0;
    ea::vector<ea::string> failures;
    const auto testStart = Clock::now();
    Watchdog watchdog;

    for (const Candidate& candidate : binary)
    {
        std::ifstream stream(candidate.path_.c_str(), std::ios::binary);
        const std::vector<char> raw((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        ea::vector<unsigned char> bytes(raw.size());
        for (size_t i = 0; i < raw.size(); ++i)
            bytes[i] = static_cast<unsigned char>(raw[i]);

        // Sampled lengths: every 16 bytes in the first 256 bytes, then every 1/64 of the size.
        ea::vector<size_t> lengths;
        for (size_t length = 0; length < 256 && length < bytes.size(); length += 16)
            lengths.push_back(length);
        for (unsigned k = 1; k < 64; ++k)
        {
            const size_t length = bytes.size() * k / 64;
            if (length >= 256 && (lengths.empty() || length > lengths.back()))
                lengths.push_back(length);
        }

        unsigned numOkFile = 0;
        unsigned numErrorFile = 0;
        for (const size_t length : lengths)
        {
            ea::vector<unsigned char> truncated(bytes.begin(), bytes.begin() + length);
            const ea::string name = Format("{} @ {} of {} bytes", candidate.name_, length, bytes.size());
            if (config.report_)
            {
                std::fprintf(stderr, "ufbx-stress: begin %s\n", name.c_str());
                std::fflush(stderr);
            }
            watchdog.Begin(name);
            const ImportResult result = ImportOnce(context, "", &truncated);
            watchdog.End();
            ++numImports;

            if (result.status_ == ImportStatus::Ok)
            {
                ++numOk;
                ++numOkFile;
            }
            else if (result.status_ == ImportStatus::Error)
            {
                ++numCleanError;
                ++numErrorFile;
            }
            else
                failures.push_back(name + ": exception escaped: " + result.message_);
            if (!result.violation_.empty())
                failures.push_back(name + ": " + result.violation_);
            if (result.seconds_ > TimeLimitSeconds)
                failures.push_back(name + Format(": import took {:.1f} s", result.seconds_));
        }
        if (config.report_)
        {
            std::printf("%s\tsize=%llu\tlengths=%u\tok=%u\terror=%u\n", candidate.name_.c_str(),
                (unsigned long long)candidate.size_, (unsigned)lengths.size(), numOkFile, numErrorFile);
            std::fflush(stdout);
        }
    }

    if (config.report_)
    {
        const double total = std::chrono::duration<double>(Clock::now() - testStart).count();
        std::printf("ufbx-stress: files=%u imports=%u ok=%u clean-error=%u failures=%u time=%.1fs\n",
            (unsigned)binary.size(), numImports, numOk, numCleanError, (unsigned)failures.size(), total);
        std::fflush(stdout);
    }

    for (const ea::string& message : failures)
        FAIL_CHECK(message.c_str());
}
