
#include "../include/crawler/Request.h"
#include <assert.h>
#include <openssl/ssl.h>
// Simulates the behavior of a crawler
int main()
    {
    SSL_library_init();
    //const char * const exampleUrl = "https://umich.edu/";
    const char * const exampleUrl = "https://en.wikipedia.org\#searchInput";
    //https://news.ycombinator.com/item?id=26839781

    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    

    //assert( result.status == getReqStatus::successful  );
    // Get results
    APESEARCH::vector<char> buff = requester.getResponseBuffer();

    write(1, buff.begin(), buff.size());
    } // end main() https://news.ycombinator.comP