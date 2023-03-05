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

#include "../..//Monetization/BillingManager.h"
#include "../../Monetization/WindowsStore/BillingManager.h"

#include "../../Container/Str.h"
#include "../../Core/IteratorRange.h"
#include "../../Core/WorkQueue.h"
#include "../../IO/Log.h"
#include "../../IO/MemoryBuffer.h"
#include "../../Monetization/BillingManager.h"
#include "../../Resource/XMLFile.h"

#include <rpc.h>

#include <EASTL/functional.h>

#include <iomanip>
#include <sstream>

#include <Windows.ApplicationModel.store.h>
#include <windows.data.xml.dom.h>
#include <windows.foundation.h>

namespace Urho3D
{
namespace Platform
{

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;

namespace
{

std::chrono::system_clock::time_point FromWindows(DateTime dateTime)
{
    static const auto baseTime = []()
    {
        std::tm tm{};
        tm.tm_year = 1601;
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }();

    return baseTime + std::chrono::microseconds{dateTime.UniversalTime / 10};
}

std::chrono::system_clock::time_point ParseUTCTime(ea::string_view str)
{
    std::tm tm;
    std::istringstream ss(std::string{str.data(), str.size()});
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return std::chrono::system_clock::from_time_t(mktime(&tm));
}

BillingProductType FromWindows(Store::ProductType productType)
{
    switch (productType)
    {
    case Store::ProductType::Unknown: return BillingProductType::Unknown;
    case Store::ProductType::Durable: return BillingProductType::Durable;
    case Store::ProductType::Consumable: return BillingProductType::Consumable;
    default: return BillingProductType::Unknown;
    }
}

BillingPurchaseState FromWindows(Store::ProductPurchaseStatus status)
{
    switch (status)
    {
    case Store::ProductPurchaseStatus::Succeeded: return BillingPurchaseState::Purchased;
    case Store::ProductPurchaseStatus::AlreadyPurchased: return BillingPurchaseState::Purchased;
    case Store::ProductPurchaseStatus::NotFulfilled: return BillingPurchaseState::Deferred;
    case Store::ProductPurchaseStatus::NotPurchased: return BillingPurchaseState::Canceled;
    default: return BillingPurchaseState::Unknown;
    }
}

ea::optional<BillingError> ErrorFromFulfillmentResult(Store::FulfillmentResult fulfillmentResult)
{
    switch (fulfillmentResult)
    {
    case Store::FulfillmentResult::Succeeded: return ea::nullopt;
    case Store::FulfillmentResult::NothingToFulfill: return BillingError::ItemUnavailable;
    case Store::FulfillmentResult::PurchasePending: return BillingError::UnspecifiedError;
    case Store::FulfillmentResult::PurchaseReverted: return BillingError::UnspecifiedError;
    case Store::FulfillmentResult::ServerError: return BillingError::StoreUnavailable;
    default: return BillingError::UnspecifiedError;
    }
}

ea::optional<BillingPurchaseVector> PurchasesFromReceipt(
    ::Platform::String ^ receipt, Store::ProductPurchaseStatus status)
{
    auto xmlDocument = ref new Windows::Data::Xml::Dom::XmlDocument();
    try
    {
        xmlDocument->LoadXml(receipt);
    }
    catch (::Platform::Exception ^)
    {
        return ea::nullopt;
    }

    auto xmlProductReceipts = xmlDocument->GetElementsByTagName("ProductReceipt");

    BillingPurchaseVector purchases;
    for (unsigned i = 0; i < xmlProductReceipts->Length; ++i)
    {
        auto xmlProductReceipt = xmlProductReceipts->GetAt(i);
        auto xmlId = xmlProductReceipt->Attributes->GetNamedItem("Id");
        auto xmlPurchaseDate = xmlProductReceipt->Attributes->GetNamedItem("PurchaseDate");
        auto xmlProductId = xmlProductReceipt->Attributes->GetNamedItem("ProductId");
        if (!xmlId || !xmlPurchaseDate || !xmlProductId)
            continue;

        const ea::string purchaseDateString = WideToMultiByte(xmlPurchaseDate->InnerText->Data());

        BillingPurchase& purchase = purchases.emplace_back();
        purchase.id_ = WideToMultiByte(xmlId->InnerText->Data());
        purchase.productId_ = WideToMultiByte(xmlProductId->InnerText->Data());
        purchase.transactionDate_ = ParseUTCTime(purchaseDateString);
        purchase.state_ = FromWindows(status);

        purchase.transactionId_ = purchase.id_;
        purchase.productIds_ = {purchase.productId_};
    }
    return purchases;
}

auto LoadListingInformationAsync(bool useSimulator)
{
    return useSimulator //
        ? Store::CurrentAppSimulator::LoadListingInformationAsync()
        : Store::CurrentApp::LoadListingInformationAsync();
}

auto GetAppReceiptAsync(bool useSimulator)
{
    return useSimulator //
        ? Store::CurrentAppSimulator::GetAppReceiptAsync()
        : Store::CurrentApp::GetAppReceiptAsync();
}

auto RequestProductPurchaseAsync(bool useSimulator, ::Platform::String ^ productId)
{
    return useSimulator //
        ? Store::CurrentAppSimulator::RequestProductPurchaseAsync(productId)
        : Store::CurrentApp::RequestProductPurchaseAsync(productId);
}

auto ReportConsumableFulfillmentAsync(bool useSimulator, ::Platform::String ^ productId, ::Platform::Guid transactionId)
{
    return useSimulator //
        ? Store::CurrentAppSimulator::ReportConsumableFulfillmentAsync(productId, transactionId)
        : Store::CurrentApp::ReportConsumableFulfillmentAsync(productId, transactionId);
}

template <class T> void OnCompleted(IAsyncOperation<T> ^ task, ea::function<void(T result, bool success)>&& callback)
{
    auto wrappedCallback = [callback = ea::move(callback)](
                               IAsyncOperation<T> ^ task, Windows::Foundation::AsyncStatus status)
    {
        if (status == Windows::Foundation::AsyncStatus::Completed)
            callback(task->GetResults(), true);
        else
            callback(T(), false);
    };

    task->Completed = ref new AsyncOperationCompletedHandler<T>(wrappedCallback);
}

} // namespace

BillingManagerUWP::BillingManagerUWP(Context* context)
    : BillingManager(context)
{
    // TODO: Remove
    GetProductsAsync({"1", "2"},
        [](const ea::optional<BillingProductVector>& result)
        {
        // result.has_value();
    });
    GetPurchasesAsync(
        [=](const ea::optional<BillingPurchaseVector>& result)
        {
        auto workQueue = GetSubsystem<WorkQueue>();
        auto t = result->at(1);
        workQueue->CallFromMainThread(
            [=](unsigned) { ConsumeAsync(t.productId_, t.transactionId_, [](ea::optional<BillingError>) {}); });
    });
    if (0)
        PurchaseAsync("1",
            [](const ea::optional<BillingPurchase>& result)
            {
            // result.has_value();
        });
}

BillingManagerUWP::~BillingManagerUWP()
{
}

void BillingManagerUWP::GetProductsAsync(const ea::vector<ea::string>& productIds, const OnProductsReceived& callback)
{
    auto task = LoadListingInformationAsync(simulatorEnabled_);
    OnCompleted<Store::ListingInformation ^>(task,
        [=](Store::ListingInformation ^ listingInformation, bool success)
        {
        if (!success || !listingInformation)
        {
            callback(ea::nullopt);
            return;
        }

        BillingProductVector products;

        for (const ea::string& productId : productIds)
        {
            auto wrappedProductId = ref new ::Platform::String(MultiByteToWide(productId).c_str());
            if (!listingInformation->ProductListings->HasKey(wrappedProductId))
                continue;

            Store::ProductListing ^ productListing = listingInformation->ProductListings->Lookup(wrappedProductId);

            BillingProduct& billingProduct = products.emplace_back();
            billingProduct.name_ = WideToMultiByte(productListing->Name->Data());
            billingProduct.description_ = WideToMultiByte(productListing->Description->Data());
            billingProduct.productId_ = WideToMultiByte(productListing->ProductId->Data());
            billingProduct.formattedPrice_ = WideToMultiByte(productListing->FormattedPrice->Data());
            billingProduct.productType_ = FromWindows(productListing->ProductType);

            if (productListing->ImageUri)
                billingProduct.windows_.imageUri_ = WideToMultiByte(productListing->ImageUri->AbsoluteUri->Data());
            billingProduct.windows_.tag_ = WideToMultiByte(productListing->Tag->Data());
            for (auto iter = productListing->Keywords->First(); iter->HasCurrent; iter->MoveNext())
                billingProduct.windows_.keywords_.push_back(WideToMultiByte(iter->Current->Data()));

            // These fields need "Windows.Foundation.UniversalApiContract version 2.0"
            // billingProduct.currencyCode_ = WideToMultiByte(productListing->CurrencyCode->Data());
            // billingProduct.windows_.formattedBasePrice_ =
            // WideToMultiByte(productListing->FormattedBasePrice->Data()); billingProduct.windows_.isOnSale_ =
            // productListing->IsOnSale.ToString(); billingProduct.windows_.saleEndDate_ =
            // FromWindows(productListing->SaleEndDate);
        }

        ea::optional<BillingProductVector> result = ea::move(products);
        callback(result);
    });
}

void BillingManagerUWP::GetPurchasesAsync(const OnPurchasesReceived& callback)
{
    auto task = GetAppReceiptAsync(simulatorEnabled_);
    OnCompleted<::Platform::String ^>(task,
        [=](::Platform::String ^ receipt, bool success)
        {
        if (!success || !receipt)
        {
            callback(ea::nullopt);
            return;
        }

        const auto purchases = PurchasesFromReceipt(receipt, Store::ProductPurchaseStatus::AlreadyPurchased);
        callback(purchases);
    });
}

void BillingManagerUWP::PurchaseAsync(const ea::string& productId, BillingProductType productType, const ea::string& obfuscatedAccountId, const ea::string& obfuscatedProfileId)
{
    const auto wrappedProductId = ref new ::Platform::String(MultiByteToWide(productId).c_str());
    auto task = RequestProductPurchaseAsync(simulatorEnabled_, wrappedProductId);
    OnCompleted<Store::PurchaseResults ^>(task,
        [=](Store::PurchaseResults ^ purchaseResults, bool success)
        {
        if (!success || !purchaseResults)
        {
            callback(ea::nullopt);
            return;
        }

        if (purchaseResults->ReceiptXml)
        {
            const auto purchases = PurchasesFromReceipt(purchaseResults->ReceiptXml, purchaseResults->Status);
            if (!purchases || purchases->empty())
            {
                callback(ea::nullopt);
                return;
            }

            callback(purchases->front());
        }
        else
        {

            BillingPurchase purchase;

            purchase.id_ = WideToMultiByte(purchaseResults->TransactionId.ToString()->Data());
            // purchase.offerId_ = WideToMultiByte(purchaseResults->OfferId->Data());
            purchase.productId_ = productId;
            purchase.transactionDate_ = std::chrono::system_clock::now();

            purchase.transactionId_ = purchase.id_;
            purchase.productIds_ = {purchase.productId_};

            purchase.state_ = FromWindows(purchaseResults->Status);

            callback(purchase);
        }
    });
}

void BillingManagerUWP::ConsumeAsync(
    const ea::string& productId, const ea::string& transactionId, const OnPurchaseConsumed& callback)
{
    const auto wrappedProductId = ref new ::Platform::String(MultiByteToWide(productId).c_str());

    GUID transactionGuid;
    const RPC_STATUS status =
        ::UuidFromStringA(reinterpret_cast<RPC_CSTR>(const_cast<char*>(transactionId.c_str())), &transactionGuid);
    if (status != 0)
    {
        callback(BillingError::UnspecifiedError);
        return;
    }

    ::Platform::Guid wrappedTransactionGuid(transactionGuid);
    auto task = ReportConsumableFulfillmentAsync(simulatorEnabled_, wrappedProductId, wrappedTransactionGuid);
    OnCompleted<Store::FulfillmentResult>(task,
        [=](Store::FulfillmentResult fulfillmentResult, bool success)
        {
        ea::optional<BillingError> error;
        if (!success)
            error = BillingError::UnspecifiedError;
        else
            error = ErrorFromFulfillmentResult(fulfillmentResult);
        callback(error);
    });
}

} // namespace Platform

} // namespace Urho3D
