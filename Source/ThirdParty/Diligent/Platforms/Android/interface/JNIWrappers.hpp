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

#include <jni.h>

namespace Diligent
{

namespace JNIWrappers
{

class Method
{
public:
    Method(JNIEnv& env, jclass cls, const char* name, const char* sig) :
        env_{env},
        mid_{env.GetMethodID(cls, name, sig)}
    {
    }

    template <typename RetType, typename... Args>
    RetType Call(jobject obj, Args&&... args) const
    {
        return *this ?
            static_cast<RetType>(env_.CallObjectMethod(obj, mid_, std::forward<Args>(args)...)) :
            RetType{nullptr};
    }

    explicit operator bool() const
    {
        return mid_ != nullptr;
    }

private:
    JNIEnv&   env_;
    jmethodID mid_ = nullptr;
};

class Clazz
{
public:
    Clazz(JNIEnv& env, const char* class_name) :
        env_{env},
        cls_{class_name != nullptr ? env_.FindClass(class_name) : nullptr}
    {
    }

    Clazz(Clazz&& other) :
        env_{other.env_},
        cls_{other.cls_}
    {
        other.cls_ = nullptr;
    }

    Clazz(Clazz& other) = delete;

    ~Clazz()
    {
        if (cls_ != nullptr)
        {
            env_.DeleteLocalRef(cls_);
        }
    }

    Method GetMethod(const char* name, const char* sig) const
    {
        return Method{env_, cls_, name, sig};
    }

    explicit operator bool() const
    {
        return cls_ != nullptr;
    }

protected:
    JNIEnv& env_;
    jclass  cls_ = nullptr;
};

template <typename ObjType = jobject>
class Object : public Clazz
{
public:
    Object(JNIEnv& env, ObjType obj, const char* class_name = nullptr) :
        Clazz{env, class_name},
        obj_{obj}
    {
    }

    Object(Object&& other) :
        Clazz(std::move(static_cast<Clazz&>(other))),
        obj_{other.obj_}
    {
        other.obj_ = nullptr;
    }

    Object(Object& other) = delete;

    ~Object()
    {
        if (obj_ != nullptr)
        {
            env_.DeleteLocalRef(obj_);
        }
    }

    ObjType Detach()
    {
        ObjType obj = obj_;
        obj_        = nullptr;
        return obj;
    }

    explicit operator bool() const
    {
        return static_cast<const Clazz&>(*this) && (obj_ != nullptr);
    }

protected:
    ObjType obj_ = nullptr;
};

class String : public Object<jstring>
{
public:
    String(JNIEnv& env, jstring str_obj) :
        Object{env, str_obj}
    {
    }

    std::string GetStdString() const
    {
        const char* chars = env_.GetStringUTFChars(obj_, nullptr);
        std::string str{chars};
        env_.ReleaseStringUTFChars(obj_, chars);
        return str;
    }

    explicit operator bool() const
    {
        // Clazz is null
        return obj_ != nullptr;
    }
};

class File : public Object<>
{
public:
    File(JNIEnv& env, jobject file_obj) :
        Object{env, file_obj, "java/io/File"},
        get_path_method_{env_, cls_, "getPath", "()Ljava/lang/String;"}
    {
    }

    String GetPath() const
    {
        return String{env_, get_path_method_.Call<jstring>(obj_)};
    }

private:
    Method get_path_method_;
};

} // namespace JNIWrappers

} // namespace Diligent
