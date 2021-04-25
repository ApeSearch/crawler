
#include "../include/crawler/Request.h"
#include <assert.h>
#include <openssl/ssl.h>
// Simulates the behavior of a crawlers
int main()
    {
    SSL_library_init();
    //const char * const exampleUrl = "https://umich.edu/";
    //const char * const exampleUrl = "https://web.archive.org/web/20070614025835/http://www.unhcr.org/publ/PUBL/4492677f0.pdf";
    const char * const exampleUrl = "https://odp.org/";
    //https://news.ycombinator.com/item?id=26854819

    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    

    //assert( result.status == getReqStatus::notHtml  );
    int num = ( int ) result.status;
    std::cout << num << std::endl;
    // Get results
    APESEARCH::vector<char> buff = requester.getResponseBuffer();

    write(1, buff.begin(), buff.size());
    } // end main() https://news.ycombinator.comP