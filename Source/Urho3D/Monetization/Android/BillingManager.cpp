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

#include "../../Core/WorkQueue.h"
#include "../../Resource/XMLFile.h"

#include "../../Monetization/Android/Activity.h"
#include "../../Monetization/Android/AppContext.h"
#include "../../Monetization/Android/BillingClient.h"
#include "../../Monetization/Android/BillingClientBuilder.h"
#include "../../Monetization/Android/BillingClientStateListener.h"
#include "../../Monetization/Android/PurchasesUpdatedListener.h"
#include "../../Monetization/Android/RbfxBillingClientStateListener.h"
#include "../../Monetization/Android/RbfxPurchasesUpdatedListener.h"

#include <EASTL/functional.h>
#include <SDL_system.h>
#include <jni.h>

namespace Urho3D
{
namespace Platform
{

BillingManagerAndroid::BillingManagerAndroid(Context* context)
    :BillingManager(context)
{
    ConnectAsync(true);
}

void BillingManagerAndroid::ConnectAsync(bool enablePendingPurchases)
{
    JNIEnv& env = *(JNIEnv*)SDL_AndroidGetJNIEnv();

    // Get a reference to the Activity class
    static auto& activityClass = jni::Class<Activity>::Singleton(env);
    static auto& rbfxPurchasesUpdatedListenerClass = jni::Class<RbfxPurchasesUpdatedListener>::Singleton(env);
    static auto& rbfxBillingClientStateListenerClass = jni::Class<RbfxBillingClientStateListener>::Singleton(env);
    static auto& billingClientStateListenerClass = jni::Class<BillingClientStateListener>::Singleton(env);
    static auto& purchasesUpdatedListenerClass = jni::Class<PurchasesUpdatedListener>::Singleton(env);

    // Get the Context object from the current activity
    auto currentActivity = jni::Local<jni::Object<Activity>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef((jobject)SDL_AndroidGetActivity())));
    auto contextObject = Activity::GetApplicationContext(env, currentActivity);

    // Create a new instance of RbfxPurchasesUpdatedListener
    auto rbfxPurchasesUpdatedListener = RbfxPurchasesUpdatedListener::Create(env, this);

    // Build the BillingClient object
    auto builder = BillingClient::NewBuilder(env, contextObject);
    builder = BillingClientBuilder::SetListener(env, builder, jni::Cast(env, purchasesUpdatedListenerClass, rbfxPurchasesUpdatedListener));
    builder = BillingClientBuilder::EnablePendingPurchases(env, builder);
    auto billingClientObject = BillingClientBuilder::Build(env, builder);

    // Create a new instance of RbfxPurchasesUpdatedListener
    auto rbfxBillingClientStateListener = RbfxBillingClientStateListener::Create(env, this);

    BillingClient::StartConnection(env, billingClientObject, jni::Cast(env, billingClientStateListenerClass, rbfxBillingClientStateListener));
}

void BillingManagerAndroid::DisconnectAsync()
{
}

void BillingManagerAndroid::BillingServiceDisconnected()
{
}

void BillingManagerAndroid::BillingSetupFinished(BillingResultCode code, const ea::string& debugMessage)
{
    if (code != BillingResultCode::OK)
        URHO3D_LOGERROR("BillingSetupFinished with error, code {}, message {}", code, debugMessage);
    else
        URHO3D_LOGDEBUG("BillingSetupFinished, code {}, message {}", code, debugMessage);
}


void BillingManagerAndroid::GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback)
{
    callback(ea::nullopt);
}

void BillingManagerAndroid::GetPurchasesAsync(const OnPurchasesReceived& callback)
{
    callback(ea::nullopt);
}

void BillingManagerAndroid::PurchaseAsync(const ea::string& productId, const OnPurchaseProcessed& callback)
{
    callback(ea::nullopt);
}

void BillingManagerAndroid::ConsumeAsync(
        const ea::string& purchaseId, const ea::string& transactionId, const OnPurchaseConsumed& callback)
{
    callback(BillingError::UnspecifiedError);
}

} // namespace Platform

} // namespace Urho3D
