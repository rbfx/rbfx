#include "../CommonUtils.h"

#include <Urho3D/Network/URL.h>


TEST_CASE("URL::Encode and URL::Decode")
{
    REQUIRE(URL::Encode("Abcd123!@#$%^&*()_+-={}|\":<>?[]';,./~`") == "Abcd123%21%40%23%24%25%5E%26%2A%28%29_%2B-%3D%7B%7D%7C%22%3A%3C%3E%3F%5B%5D%27%3B%2C.%2F~%60");
    REQUIRE(URL::Decode("Abcd123%21%40%23%24%25%5E%26%2A%28%29_%2B-%3D%7B%7D%7C%22%3A%3C%3E%3F%5B%5D%27%3B%2C.%2F~%60") == "Abcd123!@#$%^&*()_+-={}|\":<>?[]';,./~`");
}

TEST_CASE("URL assembly and disassembly")
{
    REQUIRE(!URL());
    URL url("foo://beak:sharp@example.com:8042/over/there?name=ferret#nose");
    REQUIRE(!!url);
    REQUIRE(url.scheme_ == "foo");
    REQUIRE(url.user_ == "beak");
    REQUIRE(url.password_ == "sharp");
    REQUIRE(url.host_ == "example.com");
    REQUIRE(url.port_ == 8042);
    REQUIRE(url.path_ == "/over/there");
    REQUIRE(url.query_ == "name=ferret");
    REQUIRE(url.hash_ == "nose");
    REQUIRE(url.ToString() == "foo://beak:sharp@example.com:8042/over/there?name=ferret#nose");
}
