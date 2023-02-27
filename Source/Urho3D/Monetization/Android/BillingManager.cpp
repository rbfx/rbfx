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

#include <EASTL/functional.h>
#include <SDL_system.h>
#include <jni.h>

extern "C"
JNIEXPORT void JNICALL Java_io_urho3d_RbfxBillingClientStateListener_onBillingServiceDisconnected(JNIEnv *env, jobject obj)
{
}

extern "C"
JNIEXPORT void JNICALL Java_io_urho3d_RbfxBillingClientStateListener_onBillingSetupFinished(JNIEnv *env, jobject obj, jobject billingResult)
{
}

extern "C"
JNIEXPORT void JNICALL Java_io_urho3d_RbfxPurchasesUpdatedListener_onPurchasesUpdated(JNIEnv *env, jobject obj, jobject billingResult, jobjectArray purchases)
{
    // Iterate over the purchases and convert them to C++ objects
    for (int i = 0; i < env->GetArrayLength(purchases); i++)
    {
        jobject purchaseObject = env->GetObjectArrayElement(purchases, i);

        // Get the Purchase class and method IDs
        jclass purchaseClass = env->GetObjectClass(purchaseObject);
        jmethodID getOrderIdMethod = env->GetMethodID(purchaseClass, "getOrderId", "()Ljava/lang/String;");
        jmethodID getProductsMethod = env->GetMethodID(purchaseClass, "getProducts", "()Ljava/util/List;");
        jmethodID getPurchaseTimeMethod = env->GetMethodID(purchaseClass, "getPurchaseTime", "()J");
        jmethodID getPurchaseStateMethod = env->GetMethodID(purchaseClass, "getPurchaseState", "()I");
        jmethodID getDeveloperPayloadMethod = env->GetMethodID(purchaseClass, "getDeveloperPayload", "()Ljava/lang/String;");
        jmethodID getPurchaseTokenMethod = env->GetMethodID(purchaseClass, "getPurchaseToken", "()Ljava/lang/String;");

        // Call the Purchase class methods to get the purchase details
        jstring orderIdObject = (jstring) env->CallObjectMethod(purchaseObject, getOrderIdMethod);
        jobjectArray products = (jobjectArray) env->CallObjectMethod(purchaseObject, getProductsMethod);
        jlong purchaseTime = env->CallLongMethod(purchaseObject, getPurchaseTimeMethod);
        jint purchaseState = env->CallIntMethod(purchaseObject, getPurchaseStateMethod);
        jstring developerPayloadObject = (jstring) env->CallObjectMethod(purchaseObject, getDeveloperPayloadMethod);
        jstring purchaseTokenObject = (jstring) env->CallObjectMethod(purchaseObject, getPurchaseTokenMethod);

        // Convert the jstrings to C strings
        const char* orderId = env->GetStringUTFChars(orderIdObject, NULL);
        const char* developerPayload = env->GetStringUTFChars(developerPayloadObject, NULL);
        const char* purchaseToken = env->GetStringUTFChars(purchaseTokenObject, NULL);

        // Release the jstrings
        env->ReleaseStringUTFChars(orderIdObject, orderId);
        env->ReleaseStringUTFChars(developerPayloadObject, developerPayload);
        env->ReleaseStringUTFChars(purchaseTokenObject, purchaseToken);

        // Release the local references
        env->DeleteLocalRef(orderIdObject);
        env->DeleteLocalRef(developerPayloadObject);
        env->DeleteLocalRef(purchaseTokenObject);
        env->DeleteLocalRef(purchaseObject);
        env->DeleteLocalRef(purchaseClass);
    }
}

namespace Urho3D
{
namespace Platform
{
BillingManagerAndroid::BillingManagerAndroid(Context* context)
    :BillingManager(context)
{
        //ConnectAsync(true, [](bool a){});
}

void BillingManagerAndroid::ConnectAsync(bool enablePendingPurchases, const OnConnected& callback)
{
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

    // Get a reference to the Activity class
    jclass activityClass = env->FindClass("android/app/Activity");

    // Get a reference to the current activity object (assuming this is being called from an activity)
    jobject currentActivity = (jobject)SDL_AndroidGetActivity();

    // Get the Context object from the current activity
    jmethodID getApplicationContextMethod = env->GetMethodID(activityClass, "getApplicationContext", "()Landroid/content/Context;");
    jobject contextObject = env->CallObjectMethod(currentActivity, getApplicationContextMethod);

    // Get a reference to the BillingClient class
    jclass billingClientClass = env->FindClass("com/android/billingclient/api/BillingClient");

    // Call the static NewBuilder method
    jmethodID newBuilderMethod = env->GetStaticMethodID(billingClientClass, "newBuilder", "(Landroid/content/Context;)Lcom/android/billingclient/api/BillingClient$Builder;");
    jobject builderObject = env->CallStaticObjectMethod(billingClientClass, newBuilderMethod, contextObject);

    // Create a new instance of RbfxPurchasesUpdatedListener
    jclass rbfxPurchasesUpdatedListenerClass  = env->FindClass("io/urho3d/RbfxPurchasesUpdatedListener");
    jmethodID rbfxPurchasesUpdatedListenerClassConstructor = env->GetMethodID(rbfxPurchasesUpdatedListenerClass, "<init>", "()V");
    jobject purchasesUpdatedListener = env->NewObject(rbfxPurchasesUpdatedListenerClass, rbfxPurchasesUpdatedListenerClassConstructor);

    // Call the setListener method of the BillingClient.Builder object with the RbfxPurchasesUpdatedListener object
    jmethodID setListenerMethod = env->GetMethodID(env->GetObjectClass(builderObject), "setListener", "(Lcom/android/billingclient/api/PurchasesUpdatedListener;)Lcom/android/billingclient/api/BillingClient$Builder;");
    jobject builderWithListenerObject = env->CallObjectMethod(builderObject, setListenerMethod, purchasesUpdatedListener);

    // Configure the builder object
    jmethodID enablePendingPurchasesMethod = env->GetMethodID(env->GetObjectClass(builderObject), "enablePendingPurchases", "()Lcom/android/billingclient/api/BillingClient$Builder;");
    env->CallObjectMethod(builderObject, enablePendingPurchasesMethod);

    // Build the BillingClient object
    jmethodID buildMethod = env->GetMethodID(env->GetObjectClass(builderObject), "build", "()Lcom/android/billingclient/api/BillingClient;");
    jobject billingClientObject = env->CallObjectMethod(builderObject, buildMethod);

    // Call the StartConnection method
    jmethodID startConnectionMethod = env->GetMethodID(env->GetObjectClass(billingClientObject), "startConnection",
                                                       "(Lcom/android/billingclient/api/BillingClientStateListener;)V");

    // Create a new instance of RbfxBillingClientStateListener
    jclass rbfxBillingClientStateListenerClass = env->FindClass("io/urho3d/RbfxBillingClientStateListener");
    jmethodID rbfxBillingClientStateListenerClassConstructor = env->GetMethodID(rbfxBillingClientStateListenerClass, "<init>", "()V");
    jobject billingClientStateListener = env->NewObject(rbfxBillingClientStateListenerClass, rbfxBillingClientStateListenerClassConstructor);

    // Call the StartConnection method with the BillingClientStateListener object
    env->CallVoidMethod(billingClientObject, startConnectionMethod, billingClientStateListener);

    callback(true);
}

void BillingManagerAndroid::DisconnectAsync(const OnDisconnected& callback)
{
    callback();
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
