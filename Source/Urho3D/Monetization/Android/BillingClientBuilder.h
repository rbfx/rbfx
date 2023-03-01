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

#include <jni/jni.hpp>

namespace Urho3D
{
namespace Platform
{
struct BillingClient;
struct PurchasesUpdatedListener;
struct BillingClientStateListener;

/// JNI.HPP wrapper for the BillingClientBuilder java class.
struct BillingClientBuilder {
    static constexpr auto Name() { return "com/android/billingclient/api/BillingClient$Builder"; }

    static void registerNative(jni::JNIEnv &env) { jni::Class<BillingClientBuilder>::Singleton(env); }

    static jni::Local <jni::Object<BillingClient>> Build(jni::JNIEnv &env, const jni::Object<BillingClientBuilder>&);

    static jni::Local <jni::Object<BillingClientBuilder>> SetListener(jni::JNIEnv &env, const jni::Object<BillingClientBuilder>&builder, const jni::Object<PurchasesUpdatedListener>& listener);

    static jni::Local <jni::Object<BillingClientBuilder>> EnablePendingPurchases(jni::JNIEnv &env, const jni::Object<BillingClientBuilder>&builder);
};

}
}
