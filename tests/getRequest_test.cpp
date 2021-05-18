
#include "../include/crawler/Request.h"
#include <assert.h>
#include <openssl/ssl.h>
// Simulates the behavior of a crawlers
int main(int argc, char **argv )
    {
    SSL_library_init();
    //const char * const exampleUrl = "https://umich.edu/";
    //const char * const exampleUrl = "https://web.archive.org/web/20070614025835/http://www.unhcr.org/publ/PUBL/4492677f0.pdf";
    const char * const exampleUrl = "http://www.youtube.com/";
    //const char * const exampleUrl = "https://www.cmbchina.com";
    //const char * const exampleUrl = "https://apimagesblog.com/blog/2014/06/25/photographer-andy-wong";
    //const char * const exampleUrl = "https://news.ycombinator.com/item?id=26854819";

    //const char * const exampleUrl = argv[1];
    Request requester;
    // Figure out results
    Result result( requester.getReqAndParse( exampleUrl ) );    

    //assert( result.status == getReqStatus::notHtml  );
    //assert( result.status == getReqStatus::successful );
    int num = ( int ) result.status;
    // Get results
    APESEARCH::vector<char> buff = requester.getResponseBuffer();

    write(1, buff.begin(), buff.size());
    std::cout << "\r\nEndResponse\n";
    std::cout << num << std::endl;
    } // end main() https://news.ycombinator.com