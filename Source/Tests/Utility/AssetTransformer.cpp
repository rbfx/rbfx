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
        transformer->SetFlavor(ApplicationFlavorPattern{flavor});
        hierarchy->AddTransformer(path, transformer);
        return transformer;
    };
    auto getTransformerCandidates = [&](const ea::string& path, const ea::string& flavor = "*=*")
    {
        return hierarchy->GetTransformerCandidates(path, ApplicationFlavor{flavor});
    };

    const auto t0_o_1 = addTransformer("", "");
    const auto t0_o_2 = addTransformer("", "platform=*");
    const auto t0_M = addTransformer("", "platform=mobile");
    const auto t0_MI_1 = addTransformer("", "platform=mobile,ios");
    const auto t0_MI_2 = addTransformer("", "platform=mobile,ios");
    const auto t0_MA = addTransformer("", "platform=mobile,android");
    const auto t00_MA = addTransformer("foo", "platform=mobile,android");
    const auto t000_o = addTransformer("foo/bar", "");

    // Test extra slashes
    REQUIRE(getTransformerCandidates("/foo/bar") == getTransformerCandidates("foo/bar"));
    REQUIRE(getTransformerCandidates("foo/bar/") == getTransformerCandidates("foo/bar"));
    REQUIRE(getTransformerCandidates("/foo/bar/") == getTransformerCandidates("foo/bar"));

    // Test path queries
    REQUIRE(getTransformerCandidates("foo/bar/bun") == TestVector{
        t000_o, t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});
    REQUIRE(getTransformerCandidates("foo/bar") == TestVector{
        t000_o, t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});

    REQUIRE(getTransformerCandidates("foo/buz") == TestVector{
        t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});
    REQUIRE(getTransformerCandidates("foo") == TestVector{
        t00_MA, t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});

    REQUIRE(getTransformerCandidates("fuz") == TestVector{
        t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});
    REQUIRE(getTransformerCandidates("") == TestVector{
        t0_o_1, t0_o_2, t0_M, t0_MI_1, t0_MI_2, t0_MA});

    // Test path and flavor queries
    REQUIRE(getTransformerCandidates("foo/bar", "") == TestVector{
        t000_o, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile") == TestVector{
        t000_o, t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile,androids") == TestVector{
        t000_o, t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile,ios") == TestVector{
        t000_o, t0_MI_1, t0_MI_2, t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile,android") == TestVector{
        t000_o, t00_MA, t0_MA, t0_M, t0_o_1, t0_o_2});

    REQUIRE(getTransformerCandidates("foo", "") == TestVector{
        t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo", "platform=mobile") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo", "platform=mobile,androids") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo", "platform=mobile,ios") == TestVector{
        t0_MI_1, t0_MI_2, t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("foo", "platform=mobile,android") == TestVector{
        t00_MA, t0_MA, t0_M, t0_o_1, t0_o_2});

    REQUIRE(getTransformerCandidates("", "") == TestVector{
        t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("", "platform=mobile") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("", "platform=mobile,androids") == TestVector{
        t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("", "platform=mobile,ios") == TestVector{
        t0_MI_1, t0_MI_2, t0_M, t0_o_1, t0_o_2});
    REQUIRE(getTransformerCandidates("", "platform=mobile,android") == TestVector{
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
        transformer->SetFlavor(ApplicationFlavorPattern{flavor});
        hierarchy->AddTransformer(path, transformer);
        return transformer;
    };
    auto getTransformerCandidates = [&](const ea::string& path, const ea::string& flavor)
    {
        return hierarchy->GetTransformerCandidates(path, ApplicationFlavor{flavor});
    };

    const auto t0 = addTransformer("", "platform=*");
    const auto t1 = addTransformer("", "platform=mobile");
    const auto t2 = addTransformer("", "platform=mobile,ios");
    const auto t3 = addTransformer("foo/bar", "platform=*");

    REQUIRE(getTransformerCandidates("", "platform=*") == TestVector{t0});
    REQUIRE(getTransformerCandidates("", "platform=mobile") == TestVector{t1});
    REQUIRE(getTransformerCandidates("", "platform=mobile,ios") == TestVector{t2});

    REQUIRE(getTransformerCandidates("foo", "platform=*") == TestVector{t0});
    REQUIRE(getTransformerCandidates("foo", "platform=mobile") == TestVector{t1});
    REQUIRE(getTransformerCandidates("foo", "platform=mobile,ios") == TestVector{t2});

    REQUIRE(getTransformerCandidates("foo/bar", "platform=*") == TestVector{t3});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile") == TestVector{t3});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile,ios") == TestVector{t3});
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
        transformer->SetFlavor(ApplicationFlavorPattern{flavor});
        hierarchy->AddTransformer(path, transformer);
        return transformer;
    };
    auto getTransformerCandidates = [&](const ea::string& path, const ea::string& flavor)
    {
        return hierarchy->GetTransformerCandidates(path, ApplicationFlavor{flavor});
    };

    hierarchy->AddDependency(TestAssetTransformerB::GetTypeNameStatic(), TestAssetTransformerA::GetTypeNameStatic());
    hierarchy->AddDependency(TestAssetTransformerC::GetTypeNameStatic(), TestAssetTransformerB::GetTypeNameStatic());
    hierarchy->AddDependency(TestAssetTransformerC::GetTypeNameStatic(), TestAssetTransformerA::GetTypeNameStatic());
    hierarchy->CommitDependencies();

    const auto t0 = addTransformer(0, "", "platform=*");
    const auto t1 = addTransformer(1, "", "platform=*");
    const auto t2 = addTransformer(2, "", "platform=*");
    const auto t3 = addTransformer(1, "", "platform=mobile");
    const auto t4 = addTransformer(1, "foo/bar", "platform=*");

    REQUIRE(getTransformerCandidates("", "platform=*") == TestVector{t0, t1, t2});
    REQUIRE(getTransformerCandidates("", "platform=mobile") == TestVector{t0, t3, t2});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=*") == TestVector{t0, t4, t2});
    REQUIRE(getTransformerCandidates("foo/bar", "platform=mobile") == TestVector{t0, t4, t2});
}
