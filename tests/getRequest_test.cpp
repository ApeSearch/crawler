
#include "../include/crawler/Request.h"
#include <assert.h>
#include <openssl/ssl.h>
// Simulates the behavior of a crawlers
int main()
    {
    SSL_library_init();
    //const char * const exampleUrl = "https://umich.edu/";
    const char * const exampleUrl = "https://bj.58.com/";
    //https://news.ycombinator.com/item?id=26854819

    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    

    assert( result.status == getReqStatus::notHtml  );
    // Get results
    //APESEARCH::vector<char> buff = requester.getResponseBuffer();

    //write(1, buff.begin(), buff.size());
    } // end main() https://news.ycombinator.comP