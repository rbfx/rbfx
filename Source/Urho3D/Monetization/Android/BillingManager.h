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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../../Monetization/BillingManager.h"
#include "../../Monetization/Android/BillingResult.h"
#include "../../Monetization/Android/RbfxBillingClient.h"

#include <EASTL/string.h>

namespace Urho3D
{
namespace Platform
{
    /// Android implementation of BillingManager.
    class URHO3D_API BillingManagerAndroid : public BillingManager
    {
        public:
        BillingManagerAndroid(Context* context);

        /// Implement BillingManager.
        /// @{
        void SetSimulatorEnabled(bool enabled) override {}
        bool IsSupported() const override { return false; }

        void GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback) override;
        void GetPurchasesAsync(const OnPurchasesReceived& callback) override;
        void PurchaseAsync(const ea::string& productId, BillingProductType productType, const ea::string& obfuscatedAccountId, const ea::string& obfuscatedProfileId) override;
        void ConsumeAsync(
                const ea::string& productId, const ea::string& transactionId, const OnPurchaseConsumed& callback) override;
        /// @}

        // Executed from BillingClientStateListener.
        void BillingServiceDisconnected();
        void BillingSetupFinished(BillingResultCode code, const ea::string& debugMessage);

        private:

        jni::Global<jni::Object<RbfxBillingClient> > billingClient_;
    };

} // namespace Platform
} // namespace Urho3D
