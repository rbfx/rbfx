// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../CommonUtils.h"

#include <Urho3D/Core/WorkQueue.h>

#include <array>
#include <chrono>
#include <random>
#include <thread>

namespace
{

using SharedAtomicBool = ea::shared_ptr<std::atomic_bool>;

void PostRepeatedTask(WorkQueue* workQueue, SharedAtomicBool isRunning)
{
    workQueue->PostTask([workQueue, isRunning]()
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1ms);

        if (isRunning->load(std::memory_order_relaxed))
            PostRepeatedTask(workQueue, isRunning);
    }, TaskPriority::High);
}

} // namespace

TEST_CASE("WorkQueue tasks are waitable and completion token is reusable")
{
    using namespace std::chrono_literals;

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto* workQueue = context->GetSubsystem<WorkQueue>();

    // Emulate background load
    SharedAtomicBool isRunning = ea::make_shared<std::atomic_bool>(true);
    for (unsigned i = 0; i < 32; ++i)
        PostRepeatedTask(workQueue, isRunning);

    // Create completion tokens
    constexpr unsigned NumTasks = 32;
    std::array<WorkCompletionToken, NumTasks> completionTokens;
    for (auto& token : completionTokens)
        token = WorkQueue::CreateCompletionToken();

    // Check that completion tokens work as expected
    for (unsigned iteration = 0; iteration < 100; ++iteration)
    {
        std::atomic_uint completedTasks{};
        for (const auto& token : completionTokens)
        {
            workQueue->PostTask([&completedTasks]
            {
                std::this_thread::sleep_for(5ms);
                completedTasks.fetch_add(1, std::memory_order_relaxed);
            }, TaskPriority::High, token);
        }

        for (const auto& token : completionTokens)
            workQueue->CompleteTask(token);

        CHECK(completedTasks.load(std::memory_order_relaxed) == NumTasks);
    }

    // Graceful shutdown
    isRunning->store(false, std::memory_order_relaxed);
    workQueue->CompleteAll();
}
