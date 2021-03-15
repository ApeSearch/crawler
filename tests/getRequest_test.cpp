
#include "../include/crawler/Request.h"
#include <assert.h>

// Simulates the behavior of a crawler
int main()
    {
    const char * const exampleUrl = "https://umich.edu/";

    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    
    assert( result.status == getReqStatus::successful  );
    // Get results
    APESEARCH::pair< std::string, size_t> buff( requester.getResponseBuffer() );
    } // end main()