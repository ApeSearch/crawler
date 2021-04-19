
#include "../include/crawler/Request.h"
#include <assert.h>
#include <openssl/ssl.h>
// Simulates the behavior of a crawler
int main()
    {
    SSL_library_init();
    //const char * const exampleUrl = "https://umich.edu/";
    const char * const exampleUrl = "https://www.hypebot.com/hypebot/2015/07/bmg-gets-soul-acquires-minder-music-and-cavalcade-recordings.html+of+Brazil/";
    //https://news.ycombinator.com/item?id=26854819

    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    

    //assert( result.status == getReqStatus::successful  );
    // Get results
    APESEARCH::vector<char> buff = requester.getResponseBuffer();

    write(1, buff.begin(), buff.size());
    } // end main() https://news.ycombinator.comP