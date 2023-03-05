//
// Copyright (c) 2023-2023 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../../Monetization/Android/Activity.h"
#include "../../Monetization/Android/AppContext.h"

namespace Urho3D
{
namespace Platform
{
ea::string GetJavaStringValue(jni::JNIEnv& env, const jni::String& src);

ea::string Purchase::getDeveloperPayload(jni::JNIEnv &env, const jni::Object<Purchase>& thiz);
ea::string Purchase::getOrderId(jni::JNIEnv &env, const jni::Object<Purchase>& thiz);
ea::string Purchase::getOriginalJson(jni::JNIEnv &env, const jni::Object<Purchase>& thiz);
ea::string Purchase::getPackageName(jni::JNIEnv &env, const jni::Object<Purchase>& thiz);
int Purchase::getPurchaseState(jni::JNIEnv &env, const jni::Object<Purchase>& thiz);
long Purchase::getPurchaseTim(jni::JNIEnv &env, const jni::Object<Purchase>& thiz);
ea::string Purchase::getPurchaseToken(jni::JNIEnv &env, const jni::Object<Purchase>& thiz)
{
    static auto &thisClass = jni::Class<Purchase>::Singleton(env);
    static auto method = thisClass.GetMethod<jni::String()>(env, "getPurchaseToken");
    return GetJavaStringValue(thiz.Call(env, method));
}

int Purchase::getQuantity(jni::JNIEnv &env, const jni::Object<Purchase>& thiz)
{
    static auto &thisClass = jni::Class<Purchase>::Singleton(env);
    static auto method = thisClass.GetMethod<jni::jint()>(env, "getQuantity");
    return thiz.Call(env, method);
}


}
}

