
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Request.h"
#include "../libraries/AS/include/AS/string.h"

TEST( test_evalulateRespStatus )
{
    Request requester;

    string statStr( "HTTP/1.1 301 Moved Permanently\r\n" );
    char *ptr = statStr.begin();


    int status = requester.evalulateRespStatus( &ptr,  statStr.cend() );

    ASSERT_EQUAL( status, 301 );
    ASSERT_EQUAL( statStr, string("HTTP/1.1 301\0Moved Permanently\r\n", 0, 32) );

    statStr = "HTTP/2.1 302 Woah man\r\n";
    ptr = statStr.begin();

    status = requester.evalulateRespStatus( &ptr, statStr.cend() );
    ASSERT_EQUAL( status, -1 );

    statStr = "HTTP/1.1 302\r\n";
    ptr = statStr.begin();
    status = requester.evalulateRespStatus( &ptr, statStr.cend() );
    ASSERT_EQUAL( status, -1 );

    statStr = "HTTP/1.1 30a \r\n";
    ptr = statStr.begin();
    status = requester.evalulateRespStatus( &ptr, statStr.cend() );
    ASSERT_EQUAL( status, 30 );

    statStr = "\r\n";
    ptr = statStr.begin();
    status = requester.evalulateRespStatus( &ptr, statStr.cend() );
     ASSERT_EQUAL( status, -1 );

}


TEST_MAIN()