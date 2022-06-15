//
// Copyright (c) 2017-2021 the rbfx project.
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

#include "../CommonUtils.h"

#include <Urho3D/Utility/AssetTransformerHierarchy.h>

namespace
{

class TestAssetTransformer : public AssetTransformer
{
    URHO3D_OBJECT(TestAssetTransformer, AssetTransformer);

public:
    bool singleInstanced_{};

    using AssetTransformer::AssetTransformer;

    bool IsSingleInstanced() override { return singleInstanced_; }
};

class TestAssetTransformerA : public AssetTransformer
{
    URHO3D_OBJECT(TestAssetTransformerA, AssetTransformer);

public:
    using AssetTransformer::AssetTransformer;
};

class TestAssetTransformerB : public AssetTransformer
{
    URHO3D_OBJECT(TestAssetTransformerB, AssetTransformer);

public:
    using AssetTransformer::AssetTransformer;
};

class TestAssetTransformerC : public AssetTransformer
{
    URHO3D_OBJECT(TestAssetTransformerC, AssetTransformer);

public:
    using AssetTransformer::AssetTransformer;
};

using TestVector = ea::vector<AssetTransformer*>;

}

TEST_CASE("Asset transformer performs query by flavor and path")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto hierarchy = MakeShared<AssetTransformerHierarchy>(context);
    auto addTransformer = [&](const ea::string& path, const ea::string& flavor)
    {
        const auto transformer = MakeShared<TestAssetTransformer>(context);
        transformer->SetFlavor(flavor);
        hierarchy->AddTransformer(path, transformer);
        return transformer;
    };

    const auto t0_o_1 = addTransformer("", "");
    const auto t0_o_2 = addTransformer("", "*");
    const auto t0_M = addTransformer("", "*.mobile");
    const auto t0_MI_1 = addTransformer("", "*.mobile.ios");
    const auto t0_MI_2 = addTransformer("", "*.mobile.ios");
    const auto t0_MA = addTransformer("", "*.mobile.android");
    const auto t00_MA = addTransformer("foo", "*.mobile.android");
    const auto t000_o = addTransformer("foo/bar", "*");

    // Test extra slashes
    REQUIRE(hierarchy->GetTransformerCandidates("/foo/bar") == hierarchy->GetTransformerCandidates("foo/bar"));
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar/") == hierarchy->GetTransformerCandidates("foo/bar"));
    REQUIRE(hierarchy->GetTransformerCandidates("/foo/bar/") == hierarchy->GetTransformerCandidates("foo/bar"));

    // Test path queries
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar/bun") == TestVector{
        t000_o, t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar") == TestVector{
        t000_o, t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});

    REQUIRE(hierarchy->GetTransformerCandidates("foo/buz") == TestVector{
        t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});
    REQUIRE(hierarchy->GetTransformerCandidates("foo") == TestVector{
        t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});

    REQUIRE(hierarchy->GetTransformerCandidates("fuz") == TestVector{
        t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});
    REQUIRE(hierarchy->GetTransformerCandidates("") == TestVector{
        t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});

    // Test path and flavor queries
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*") == TestVector{
        t000_o, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile") == TestVector{
        t000_o, t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile.androids") == TestVector{
        t000_o, t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile.ios") == TestVector{
        t000_o, t0_MI_1, t0_MI_2, t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile.android") == TestVector{
        t000_o, t00_MA, t0_MA, t0_M, t0_o_1, t0_o_2});

    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*") == TestVector{
        t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*.mobile") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*.mobile.androids") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*.mobile.ios") == TestVector{
        t0_MI_1, t0_MI_2, t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*.mobile.android") == TestVector{
        t00_MA, t0_MA, t0_M, t0_o_1, t0_o_2});

    REQUIRE(hierarchy->GetTransformerCandidates("", "*") == TestVector{
        t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile.androids") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile.ios") == TestVector{
        t0_MI_1, t0_MI_2, t0_M, t0_o_1, t0_o_2});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile.android") == TestVector{
        t0_MA, t0_M, t0_o_1, t0_o_2});
}

TEST_CASE("Asset transformer filters duplicates and sorts by flavor and path")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto hierarchy = MakeShared<AssetTransformerHierarchy>(context);
    auto addTransformer = [&](const ea::string& path, const ea::string& flavor)
    {
        const auto transformer = MakeShared<TestAssetTransformer>(context);
        transformer->singleInstanced_ = true;
        transformer->SetFlavor(flavor);
        hierarchy->AddTransformer(path, transformer);
        return transformer;
    };

    const auto t0 = addTransformer("", "*");
    const auto t1 = addTransformer("", "*.mobile");
    const auto t2 = addTransformer("", "*.mobile.ios");
    const auto t3 = addTransformer("foo/bar", "*");

    REQUIRE(hierarchy->GetTransformerCandidates("", "*") == TestVector{t0});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile") == TestVector{t1});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile.ios") == TestVector{t2});

    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*") == TestVector{t0});
    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*.mobile") == TestVector{t1});
    REQUIRE(hierarchy->GetTransformerCandidates("foo", "*.mobile.ios") == TestVector{t2});

    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*") == TestVector{t3});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile") == TestVector{t3});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile.ios") == TestVector{t3});
}

TEST_CASE("Asset transformer sorts by type")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto hierarchy = MakeShared<AssetTransformerHierarchy>(context);
    auto addTransformer = [&](int type, const ea::string& path, const ea::string& flavor)
    {
        SharedPtr<AssetTransformer> transformer;
        if (type == 0)
            transformer = MakeShared<TestAssetTransformerA>(context);
        else if (type == 1)
            transformer = MakeShared<TestAssetTransformerB>(context);
        else
            transformer = MakeShared<TestAssetTransformerC>(context);
        transformer->SetFlavor(flavor);
        hierarchy->AddTransformer(path, transformer);
        return transformer;
    };

    hierarchy->AddDependency(TestAssetTransformerB::GetTypeNameStatic(), TestAssetTransformerA::GetTypeNameStatic());
    hierarchy->AddDependency(TestAssetTransformerC::GetTypeNameStatic(), TestAssetTransformerB::GetTypeNameStatic());
    hierarchy->AddDependency(TestAssetTransformerC::GetTypeNameStatic(), TestAssetTransformerA::GetTypeNameStatic());
    hierarchy->CommitDependencies();

    const auto t0 = addTransformer(0, "", "*");
    const auto t1 = addTransformer(1, "", "*");
    const auto t2 = addTransformer(2, "", "*");
    const auto t3 = addTransformer(1, "", "*.mobile");
    const auto t4 = addTransformer(1, "foo/bar", "*");

    REQUIRE(hierarchy->GetTransformerCandidates("", "*") == TestVector{t0, t1, t2});
    REQUIRE(hierarchy->GetTransformerCandidates("", "*.mobile") == TestVector{t0, t3, t2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*") == TestVector{t0, t4, t2});
    REQUIRE(hierarchy->GetTransformerCandidates("foo/bar", "*.mobile") == TestVector{t0, t4, t2});
}
