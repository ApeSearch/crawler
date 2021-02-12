#include "../ParsedUrl.h"
#include <vector>

int main() {

    std::vector<const char *> bad {
        "http://",
        "http://.",
        "http://..",
        "http://../",
        "http://?",
        "http://??",
        "http://??/",
        "http://#",
        "http://##",
        "http://##/",
        "http://foo.bar?q=Spaces should be encoded",
        "//",
        "//a",
        "///a",
        "///",
        "http:///a",
        "foo.com",
        "rdar://1234",
        "h://test",
        "http:// shouldfail.com",
        ":// should fail",
        "http://foo.bar/foo(bar)baz quux"
        "ftps://foo.bar/",
        "http://-error-.invalid/",
        "http://-a.b.co",
        "http://.www.foo.bar/",
        "http://www.foo.bar./",
        "http://.www.foo.bar./",
        // "http://a.b-.co", I believe that these are technically valid format
        // "http://0.0.0.0", so they'd fail when resolving the host/port
        // "http://3628126748", rather than when parsing the URL
    };

    std::cout << "Invalid URLS\n=======================\n";
    for (auto url: bad) {
        auto parsed = ParsedUrl(url);
        if (parsed.valid_url)
            std::cout << "Failure: " << url << "\n";
    }

    
    std::vector<const char *> good {
        "https://www.facebook.com",
        "https://www.facebook.com/",
        "http://bizboro.com/honey/mailing.html",
        "http://bizboro.com/honey/contact.html",
        "http://www.mapquest.com/maps/map.adp?country=US&addtohistory=&address=1443+memorial&city=murfreesboro&state=tn&zipcode=37129&homesubmit.x=0&homesubmit.y=0",
    }; 

    std::cout << "\nValid URLS\n=======================\n";
    for (auto url: good) {
        auto parsed = ParsedUrl(url);
        if (!parsed.valid_url)
            std::cout << "Failure: " << url << "\n";
    }
}
