//
// Copyright (c) 2017 James Montemagno / Refractored LLC
// Copyright (c) 2023 the rbfx project.
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

#include "../../Monetization/Android/BillingManager.h"
#include "../../Monetization/BillingEvents.h"

#include "../../Core/WorkQueue.h"
#include "../../Resource/XMLFile.h"

#include "../../Monetization/Android/Activity.h"
#include "../../Monetization/Android/AppContext.h"
#include "../../Monetization/Android/RbfxBillingClient.h"

#include <EASTL/functional.h>
#include <SDL_system.h>
#include <jni.h>

namespace Urho3D
{
namespace Platform
{
typedef void NOP();

ea::string GetJavaStringValue(jni::JNIEnv& env, const jni::String& src)
{
    jni::UniqueStringUTFChars str;
    bool success;
    std::tie(str, success) = jni::GetStringUTFChars(env, *src);
    if (!success)
        return EMPTY_STRING;
    return ea::string(str.get());
}

jni::Local<jni::String> MakeJavaString(jni::JNIEnv& env, const ea::string& src)
{
    jni::jstring& newStr = jni::NewStringUTF(env, src.c_str());
    return jni::Local<jni::String>(env, &newStr);
}

BillingManagerAndroid::BillingManagerAndroid(Context* context)
    :BillingManager(context)
{
    JNIEnv& env = *(JNIEnv*)SDL_AndroidGetJNIEnv();

    // Get a reference to the Activity class
    static auto& activityClass = jni::Class<Activity>::Singleton(env);
    static auto& rbfxBillingClientClass = jni::Class<RbfxBillingClient>::Singleton(env);

    // Get the current activity
    auto currentActivity = jni::Local<jni::Object<Activity>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef((jobject)SDL_AndroidGetActivity())));
    auto billingClient = RbfxBillingClient::Create(env, currentActivity, RbfxLambdaContainer::Create<NOP>(env, [=](){}));
    billingClient_ = jni::NewGlobal(env, billingClient);

    RbfxBillingClient::ConnectAsync(env, billingClient_, [=](jni::JNIEnv &env, const jni::Object<BillingResult>& billingResult){
        this->BillingSetupFinished(BillingResult::GetResponseCode(env, billingResult), BillingResult::GetDebugMessage(env, billingResult));
    });
}

void BillingManagerAndroid::BillingServiceDisconnected()
{
    context_->GetSubsystem<WorkQueue>()->CallFromMainThread(
            [=](unsigned threadId)
            {
                using namespace BillingDisconnected;
                auto& eventData = GetEventDataMap();
                eventData[P_MESSAGE] = "Disconnected";
                SendEvent(E_BILLINGDISCONNECTED, eventData);
            });
}

void BillingManagerAndroid::BillingSetupFinished(BillingResultCode code, const ea::string& debugMessage)
{
    if (code != BillingResultCode::OK) {
        URHO3D_LOGERROR(Format("BillingSetupFinished with error, code {}, message {}", code, debugMessage));
        context_->GetSubsystem<WorkQueue>()->CallFromMainThread(
                [=](unsigned threadId)
                {
                    using namespace BillingDisconnected;
                    auto& eventData = GetEventDataMap();
                    eventData[P_MESSAGE] = debugMessage;
                    SendEvent(E_BILLINGDISCONNECTED, eventData);
                });
    }
    else {
        URHO3D_LOGINFO("BillingSetupFinished");
        context_->GetSubsystem<WorkQueue>()->CallFromMainThread(
                [=](unsigned threadId)
                {
                    using namespace BillingConnected;
                    auto& eventData = GetEventDataMap();
                    eventData[P_MESSAGE] = debugMessage;
                    SendEvent(E_BILLINGCONNECTED, eventData);
                });

        PurchaseAsync("android.test.purchased", BillingProductType::Durable, "", "", [=](const ea::optional<BillingPurchase>& products){

        });
    }
}

void BillingManagerAndroid::GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback)
{
    callback(ea::nullopt);
}

void BillingManagerAndroid::GetPurchasesAsync(const OnPurchasesReceived& callback)
{
    callback(ea::nullopt);
}

void BillingManagerAndroid::PurchaseAsync(const ea::string& productId, BillingProductType productType, const ea::string& obfuscatedAccountId, const ea::string& obfuscatedProfileId, const OnPurchaseProcessed& callback)
{
    JNIEnv& env = *(JNIEnv*)SDL_AndroidGetJNIEnv();

    ea::string type;
    switch (productType)
    {
        default:
            type = "inapp"; //BillingClient.ProductType.INAPP
            break;
        case BillingProductType::Subscription:
            type = "subs"; //BillingClient.ProductType.SUBS
            break;
    }
    RbfxBillingClient::PurchaseAsync(env, billingClient_, productId, type, obfuscatedAccountId, obfuscatedProfileId, RbfxLambdaContainer::Create<NOP>(env, [=](){
        //callback(nullptr);
    }));
}

void BillingManagerAndroid::ConsumeAsync(
        const ea::string& purchaseId, const ea::string& transactionId, const OnPurchaseConsumed& callback)
{
    JNIEnv& env = *(JNIEnv*)SDL_AndroidGetJNIEnv();

    callback(BillingError::UnspecifiedError);
}

} // namespace Platform

} // namespace Urho3D
