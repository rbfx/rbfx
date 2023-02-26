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

namespace Urho3D
{

namespace Platform
{
BillingManagerAndroid::BillingManagerAndroid(Context* context)
    :BillingManager(context)
{
}

void BillingManagerAndroid::ConnectAsync(bool enablePendingPurchases, const OnConnected& callback)
{
    JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();

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
