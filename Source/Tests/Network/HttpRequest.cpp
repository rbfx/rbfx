#include "../CommonUtils.h"

#include <Urho3D/Network/HttpRequest.h>

TEST_CASE("HttpRequest response headers")
{
    SharedPtr<HttpRequest> httpRequest = MakeShared<HttpRequest>("https://httpbin.org/ip");
    while(httpRequest->GetState() != HTTP_ERROR and httpRequest->GetState() != HTTP_CLOSED){

    }
    auto headers = httpRequest->GetResponseHeaders();
    REQUIRE(headers.size()!=0);
    auto header_value = httpRequest->GetResponseHeader("Content-Type");
    REQUIRE(header_value.has_value());
    REQUIRE(header_value.value().size()!=0);
}
