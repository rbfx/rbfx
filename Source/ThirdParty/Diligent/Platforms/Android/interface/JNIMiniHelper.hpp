/*
 *  Copyright 2023 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <string>
#include <android/native_activity.h>
#include <android/asset_manager.h>

#include "JNIWrappers.hpp"

namespace Diligent
{

class JNIMiniHelper
{
public:
    JNIMiniHelper(ANativeActivity* activity, std::string activity_class_name) :
        activity_{activity},
        activity_class_name_{std::move(activity_class_name)}
    {
        VERIFY(activity != nullptr && !activity_class_name_.empty(), "Activity and class name can't be null");
    }

    ~JNIMiniHelper()
    {
    }

    // clang-format off
    JNIMiniHelper           (const JNIMiniHelper&) = delete;
    JNIMiniHelper& operator=(const JNIMiniHelper&) = delete;
    JNIMiniHelper           (JNIMiniHelper&&)      = delete;
    JNIMiniHelper& operator=(JNIMiniHelper&&)      = delete;
    // clang-format on

    enum FILES_DIR_TYPE
    {
        FILES_DIR_TYPE_EXTERNAL,
        FILES_DIR_TYPE_OUTPUT
    };
    std::string GetFilesDir(FILES_DIR_TYPE type)
    {
        if (activity_ == nullptr)
        {
            LOG_ERROR_MESSAGE("JNIMiniHelper has not been initialized. Call init() to initialize the helper");
            return "";
        }

        std::string ExternalFilesPath;
        {
            std::lock_guard<std::mutex> guard{mutex_};

            JNIEnv* env          = nullptr;
            bool    DetachThread = AttachCurrentThread(env);
            if (env != nullptr)
            {
                if (JNIWrappers::Clazz activity_cls{*env, activity_class_name_.c_str()})
                {
                    auto get_external_files_dir = [&]() {
                        JNIWrappers::Method get_external_files_dir_method = activity_cls.GetMethod("getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
                        return JNIWrappers::File{
                            *env,
                            get_external_files_dir_method.Call<jobject>(activity_->clazz, NULL)};
                    };
                    auto get_files_dir = [&]() {
                        JNIWrappers::Method get_files_dir_method = activity_cls.GetMethod("getFilesDir", "()Ljava/io/File;");
                        return JNIWrappers::File{
                            *env,
                            get_files_dir_method.Call<jobject>(activity_->clazz)};
                    };

                    JNIWrappers::File file = (type == FILES_DIR_TYPE_EXTERNAL) ?
                        get_external_files_dir() :
                        get_files_dir();
                    if (file)
                    {
                        if (JNIWrappers::String path = file.GetPath())
                        {
                            ExternalFilesPath = path.GetStdString();
                        }
                    }
                }
            }
            if (DetachThread)
                DetachCurrentThread();
        }

        return ExternalFilesPath;
    }

    /*
     * Attach current thread
     * In Android, the thread doesn't have to be 'Detach' current thread
     * as application process is only killed and VM does not shut down
     */
    bool AttachCurrentThread(JNIEnv*& env)
    {
        env = nullptr;
        if (activity_->vm->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK)
            return false; // Already attached
        activity_->vm->AttachCurrentThread(&env, nullptr);
        pthread_key_create((int32_t*)activity_, DetachCurrentThreadDtor);
        return true;
    }

    /*
     * Unregister this thread from the VM
     */
    static void DetachCurrentThreadDtor(void* p)
    {
        LOG_INFO_MESSAGE("detached current thread");
        auto* activity = reinterpret_cast<ANativeActivity*>(p);
        activity->vm->DetachCurrentThread();
    }

private:
    void DetachCurrentThread()
    {
        activity_->vm->DetachCurrentThread();
    }

    ANativeActivity* activity_ = nullptr;
    std::string      activity_class_name_;

    // mutex for synchronization
    // This class uses singleton pattern and can be invoked from multiple threads,
    // each methods locks the mutex for a thread safety
    mutable std::mutex mutex_;
};

} // namespace Diligent
