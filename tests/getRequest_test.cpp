
#include "../include/crawler/Request.h"
#include <assert.h>

// Simulates the behavior of a crawler
int main()
    {
    //const char * const exampleUrl = "https://umich.edu/";
    const char * const exampleUrl = "https://youtube.com/";
    //https://news.ycombinator.com/item?id=26839781

    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    

    //assert( result.status == getReqStatus::successful  );
    // Get results
    APESEARCH::vector<char> buff = requester.getResponseBuffer();

    write(1, buff.begin(), buff.size());
    } // end main() https://news.ycombinator.com