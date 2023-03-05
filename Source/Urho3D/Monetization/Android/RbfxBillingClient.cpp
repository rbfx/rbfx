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

#include "../../Monetization/Android/RbfxBillingClient.h"

extern "C"
JNIEXPORT void JNICALL
Java_io_rebelfork_RbfxBillingClient_onPurchasesUpdated(JNIEnv *envPtr, jobject thiz, jobject billing_result, jobject list, jobject purchases_updated)
{
using namespace Urho3D;
using namespace Urho3D::Platform;
jni::JNIEnv& env = *envPtr;

static auto &thisClass = jni::Class<RbfxBillingClient>::Singleton(env);

}

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxBillingClient_onBillingSetupFinished(JNIEnv * envPtr, jobject thiz, jobject billingResult, jobject billingSetupFinished)
{
using namespace Urho3D;
using namespace Urho3D::Platform;
jni::JNIEnv& env = *envPtr;

static auto &thisClass = jni::Class<RbfxBillingClient>::Singleton(env);

auto localBillingSetupFinished = jni::Local<jni::Object<Urho3D::Platform::RbfxLambdaContainer>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef(billingSetupFinished)));
auto localBillingResult = jni::Local<jni::Object<Urho3D::Platform::BillingResult>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef(billingResult)));

RbfxLambdaContainer::Invoke<Urho3D::Platform::OnBillingSetupFinished>(env, localBillingSetupFinished, env, localBillingResult);
}
namespace Urho3D {
namespace Platform {

jni::Local<jni::String> MakeJavaString(jni::JNIEnv& env, const ea::string& src);

jni::Local<jni::Object<RbfxBillingClient> > RbfxBillingClient::Create(jni::JNIEnv &env, const jni::Object<Activity>& activity, const jni::Object<RbfxLambdaContainer>& purchasesUpdated)
{
    static auto &thisClass = jni::Class<RbfxBillingClient>::Singleton(env);
    static auto constructor = thisClass.GetConstructor<jni::Object<Activity>, jni::Object<RbfxLambdaContainer>>(env);
    return thisClass.New(env, constructor, activity, purchasesUpdated);

}

void RbfxBillingClient::ConnectAsync(jni::JNIEnv &env, const jni::Object<RbfxBillingClient>& thiz, const ea::function<OnBillingSetupFinished>& billingSetupFinished)
{
    static auto &thisClass = jni::Class<RbfxBillingClient>::Singleton(env);
    static auto method = thisClass.GetMethod<void(jni::Object<RbfxLambdaContainer>)>(env, "ConnectAsync");
    thiz.Call(env, method, RbfxLambdaContainer::Create(env, billingSetupFinished));
}

void RbfxBillingClient::PurchaseAsync(jni::JNIEnv &env, const jni::Object<RbfxBillingClient>& thiz, const ea::string& productId, const ea::string& productType, const ea::string& obfuscatedAccountId, const ea::string& obfuscatedProfileId, const jni::Object<RbfxLambdaContainer>& callback)
{
    static auto &thisClass = jni::Class<RbfxBillingClient>::Singleton(env);
    static auto method = thisClass.GetMethod<void(jni::String, jni::String, jni::String, jni::String, jni::Object<RbfxLambdaContainer>)>(env, "PurchaseAsync");
    thiz.Call(env, method, MakeJavaString(env, productId), MakeJavaString(env, productType), MakeJavaString(env, obfuscatedAccountId), MakeJavaString(env, obfuscatedProfileId), callback);
}


}
}
