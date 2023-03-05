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

#pragma once

#include "../../Monetization/Android/Activity.h"
#include "../../Monetization/Android/RbfxLambdaContainer.h"
#include "../../Monetization/Android/BillingResult.h"

#include <jni/jni.hpp>

namespace Urho3D
{
namespace Platform
{
typedef void OnBillingSetupFinished(jni::JNIEnv &env, const jni::Object<BillingResult>& billingResult);
/// JNI.HPP wrapper for the RbfxBillingClient java class.
struct RbfxBillingClient {
    static constexpr auto Name() { return "io/rebelfork/RbfxBillingClient"; }

    static void registerNative(jni::JNIEnv &env) { jni::Class<RbfxBillingClient>::Singleton(env); }

    static jni::Local<jni::Object<RbfxBillingClient> > Create(jni::JNIEnv &env, const jni::Object<Activity>& activity, const jni::Object<RbfxLambdaContainer>& purchasesUpdated);

    static void ConnectAsync(jni::JNIEnv &env, const jni::Object<RbfxBillingClient>& thiz, const ea::function<OnBillingSetupFinished>& billingSetupFinished);

    static void PurchaseAsync(jni::JNIEnv &env, const jni::Object<RbfxBillingClient>& thiz, const ea::string& productId, const ea::string& productType, const ea::string& obfuscatedAccountId, const ea::string& obfuscatedProfileId, const jni::Object<RbfxLambdaContainer>& callback);
};

}
}
