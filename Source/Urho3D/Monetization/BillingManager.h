//
// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022 the rbfx project.
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

#include "Urho3D/Core/Object.h"

#include <EASTL/string.h>

#include <chrono>

namespace Urho3D
{

enum class BillingProductType
{
    Unknown,
    Consumable,
    Durable,
    Subscription
};

enum class BillingPurchaseState
{
    Unknown,
    Purchased,
    Canceled,
    Deferred,
};

enum class BillingError
{
    UnspecifiedError,
    StoreUnavailable,
    ItemUnavailable,
};

/// Description of a product that can be purchased.
struct BillingProduct
{
    ea::string name_;
    ea::string description_;
    ea::string productId_;
    ea::string formattedPrice_;
    ea::string currencyCode_;
    BillingProductType productType_{};

    struct WindowsExtras
    {
        ea::string formattedBasePrice_;
        ea::string imageUri_;
        bool isOnSale_{};
        std::chrono::system_clock::time_point saleEndDate_{};
        ea::string tag_;
        ea::vector<ea::string> keywords_;
    } windows_;
};

using BillingProductVector = ea::vector<BillingProduct>;

/// Purchase information.
struct BillingPurchase
{
    ea::string id_;
    ea::string transactionId_;
    std::chrono::system_clock::time_point transactionDate_;
    ea::string productId_;
    ea::vector<ea::string> productIds_;
    BillingPurchaseState state_{};
};

using BillingPurchaseVector = ea::vector<BillingPurchase>;

/// In-app purchases manager.
class URHO3D_API BillingManager
    : public Object
{
    URHO3D_OBJECT(BillingManager, Object)

public:
    using OnProductsReceived = ea::function<void(const ea::optional<BillingProductVector>& products)>;
    using OnPurchasesReceived = ea::function<void(const ea::optional<BillingPurchaseVector>& purchases)>;
    using OnPurchaseProcessed = ea::function<void(const ea::optional<BillingPurchase>& purchase)>;
    using OnPurchaseConsumed = ea::function<void(ea::optional<BillingError> error)>;

    BillingManager(Context* context);
    ~BillingManager() override;

    /// Set whether to use simulator.
    virtual void SetSimulatorEnabled(bool enabled) = 0;
    /// Return whether in-app purchases are supported.
    virtual bool IsSupported() const = 0;

    /// Return product information (asynchronously).
    virtual void GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback) = 0;
    /// Return purchase information (asynchronously).
    virtual void GetPurchasesAsync(const OnPurchasesReceived& callback) = 0;
    /// Purchase a product.
    virtual void PurchaseAsync(const ea::string& productId, const OnPurchaseProcessed& callback) = 0;
    /// Consume a purchase.
    virtual void ConsumeAsync(
        const ea::string& productId, const ea::string& transactionId, const OnPurchaseConsumed& callback) = 0;

private:
};

namespace Platform
{

/// No-op implementation of BillingManager.
class URHO3D_API BillingManagerNull : public BillingManager
{
public:
    /// Implement BillingManager.
    /// @{
    void SetSimulatorEnabled(bool enabled) override {}
    bool IsSupported() const override { return false; }

    void GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback) override;
    void GetPurchasesAsync(const OnPurchasesReceived& callback) override;
    void PurchaseAsync(const ea::string& productId, const OnPurchaseProcessed& callback) override;
    void ConsumeAsync(
        const ea::string& productId, const ea::string& transactionId, const OnPurchaseConsumed& callback) override;
    /// @}
};

#if UWP
/// UWP implementation of BillingManager.
class URHO3D_API BillingManagerUWP : public BillingManager
{
public:
    BillingManagerUWP(Context* context);
    ~BillingManagerUWP() override;

    /// Implement BillingManager.
    /// @{
    void SetSimulatorEnabled(bool enabled) override { simulatorEnabled_ = enabled; }
    bool IsSupported() const { return true; }

    void GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback) override;
    void GetPurchasesAsync(const OnPurchasesReceived& callback) override;
    void PurchaseAsync(const ea::string& productId, const OnPurchaseProcessed& callback) override;
    void ConsumeAsync(
        const ea::string& productId, const ea::string& transactionId, const OnPurchaseConsumed& callback) override;
    /// @}

private:
    bool simulatorEnabled_{true};
};
#endif

} // namespace Platform

} // namespace Urho3D
