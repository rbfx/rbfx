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

package io.rebelfork;

import java.util.*;
import android.app.Activity;
import com.android.billingclient.api.*;

// Helper class for BillingClient.
public class RbfxBillingClient implements PurchasesUpdatedListener
{
    // Constructor. Executed from native code.
    public RbfxBillingClient(Activity activity, RbfxLambdaContainer purchasesUpdated) {
        activity_ = activity;
        purchasesUpdated_ = purchasesUpdated;
        billingClient_ = BillingClient.newBuilder(activity_)
            .enablePendingPurchases()
            .setListener(this)
            .build();
    }

    // Construct and connect to the billing. Executed from native code.
    public void ConnectAsync(RbfxLambdaContainer billingSetupFinished)
    {
        //Disconnect();
        billingClient_.startConnection(new BillingClientStateListener() {
            @Override
            public void onBillingSetupFinished(BillingResult billingResult) {
                RbfxBillingClient.this.onBillingSetupFinished(billingResult, billingSetupFinished);
            }
            @Override
            public void onBillingServiceDisconnected() {
            }
        });
    };

    public void Disconnect() {
        billingClient_.endConnection();
    }

    public void PurchaseAsync(String productSku, String itemType, String obfuscatedAccountId, String obfuscatedProfileId, RbfxLambdaContainer callback)
    {
        List<QueryProductDetailsParams.Product> productList = new ArrayList<>();

        boolean product = productList.add(QueryProductDetailsParams.Product.newBuilder()
            .setProductId(productSku)
            .setProductType(itemType)
            .build());

        QueryProductDetailsParams params = QueryProductDetailsParams.newBuilder()
            .setProductList(productList)
            .build();

        billingClient_.queryProductDetailsAsync(params, new ProductDetailsResponseListener() {
                @Override
                public void onProductDetailsResponse(BillingResult billingResult, List<ProductDetails> list) {
                    final List<BillingFlowParams.ProductDetailsParams> productDetailsParamsList = new ArrayList<>();

                    for (ProductDetails productDetails: list) {
                        productDetailsParamsList.add(
                            BillingFlowParams.ProductDetailsParams.newBuilder()
                                .setProductDetails(productDetails)
                                .build());
                    }

                    BillingFlowParams billingFlowParams = BillingFlowParams.newBuilder()
                        .setProductDetailsParamsList(productDetailsParamsList)
                        .build();

                    billingClient_.launchBillingFlow(activity_, billingFlowParams);
                }
            });
    }

    @Override
    public void onPurchasesUpdated(BillingResult billingResult, List<Purchase> list) {
        RbfxBillingClient.this.onPurchasesUpdated(billingResult, list, purchasesUpdated_);
    }

    private native void onPurchasesUpdated(BillingResult billingResult, List<Purchase> list, RbfxLambdaContainer purchasesUpdated);
    private native void onBillingSetupFinished(BillingResult billingResult, RbfxLambdaContainer billingSetupFinished);

    // Billing client instance.
    private BillingClient billingClient_;
    private Activity activity_;
    private RbfxLambdaContainer purchasesUpdated_;
}
