
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Request.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/algorithms.h"

TEST( test_evalulateRespStatus )
{
    Request requester;

    APESEARCH::string statStr( "HTTP/1.1 301 Moved Permanently\r\n" );
    char *ptr = statStr.begin();


    int status = requester.evalulateRespStatus( &ptr,  statStr.cend() );

    ASSERT_EQUAL( status, 301 );
    ASSERT_EQUAL( statStr, APESEARCH::string("HTTP/1.1 301\0Moved Permanently\r\n", 0, 32) );

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

TEST( test_validateStatus )
   {
    Request requester;
    getReqStatus bad = getReqStatus::badHtml;
    ASSERT_EQUAL( requester.validateStatus( 0 ), bad );
    ASSERT_EQUAL( requester.validateStatus( 100 ), bad );
    ASSERT_EQUAL( requester.validateStatus( 200 ), getReqStatus::successful );
    ASSERT_EQUAL( requester.validateStatus( 299 ), getReqStatus::successful );
    ASSERT_EQUAL( requester.validateStatus( 300 ), getReqStatus::redirected );
    ASSERT_EQUAL( requester.validateStatus( 404 ), bad );
    ASSERT_EQUAL( requester.validateStatus( 504 ), getReqStatus::ServerIssue );
    ASSERT_EQUAL( requester.validateStatus( -1 ), bad );
   }

TEST( test_request_header )
   {
     Request requester;

    std::string header( "HTTP/1.x 200 OK\r\nTransfer-Encoding: chunked\r\nDate: Sat, 28 Nov 2009 04:36:25 GM\r\nServer: LiteSpeed\r\nConnection: close\r\nX-Powered-By: W3 Total Cache/0.8\r\nPragma: public\r\nExpires: Sat, 28 Nov 2009 05:36:25 GM\r\nEtag: \"pub1259380237;gz\"\r\nCache-Control: max-age=3600, public\r\nContent-Type: text/html; charset=UTF-8\r\nLast-Modified: Sat, 28 Nov 2009 03:50:37 GMT\r\nContent-Encoding: gzip\r\nVary: Accept-Encoding, Cookie, User-Agent\r\n\r\n" );
    requester.headerBuff = APESEARCH::vector< char >( header.end( ) - header.begin( ) ); 
    APESEARCH::copy( header.begin( ), header.end( ), &requester.headerBuff.front( ) );
    requester.resetState( );

    ASSERT_FALSE( requester.gzipped );
    ASSERT_FALSE( requester.chunked );
    ASSERT_FALSE( requester.redirect );
    Result result = requester.parseHeader( &*requester.headerBuff.end( ) );
    ASSERT_EQUAL( result.status, getReqStatus::successful );
    ASSERT_TRUE( requester.gzipped );
    ASSERT_TRUE( requester.chunked );
    ASSERT_FALSE( requester.redirect );

    header = std::string( 
    "HTTP/1.1 200 OK\r\n"
    "Date: Mon, 22 Feb 2021 12:37:44 GMT\r\n"
    "Expires: -1\r\n"
    "Cache-Control: private, max-age=0\r\n"
    "Content-Type: text/html; charset=ISO-8859-1\r\n"
    "P3P: CP=\"This is not a P3P policy! See g.co/p3phelp for more info.\"\r\n"
    "Server: gws\r\n"
    "X-XSS-Protection: 0\r\n"
    "X-Frame-Options: SAMEORIGIN\r\n"
    "Set-Cookie: 1P_JAR=2021-02-22-12; expires=Wed, 24-Mar-2021 12:37:44 GMT; path=/; domain=.google.com; Secure\r\n"
    "Set-Cookie: NID=209=Q57QitAU2g7nnSs2kS_u970x_z9PtTD7FONQH-4jFMwrBz6Qcttc5l_MAT-b9U32PJQfT5Y8JQ0xxCT4PMvU304OOSk79jEEu9HC7l0kdGWklGQcO3QQkgZ-gvaXlqcCo7VYSdOaVdIUdMEzPpBWMV5oQ20-7_tX6-3yDDZkA90; expires=Tue, 24-Aug-2021 12:37:44 GMT; path=/; domain=.google.com; HttpOnly Alt-Svc: h3-29=\":443\"; ma=2592000,h3-T051=\":443\"; ma=2592000,h3-Q050=\":443\"; ma=2592000,h3-Q046=\":443\"; ma=2592000,h3-Q043=\":443\"; ma=2592000,quic=\":443\"; ma=2592000; v=\"46,43\"\r\n"
    "Accept-Ranges: none\r\n"
    "Transfer-Encoding: gzipped\r\n"
    "Vary: Accept-Encoding\r\n"
    "Connection: close\r\n"
    "Transfer-Encoding: chunked\r\n\r\n" );
    requester.headerBuff = APESEARCH::vector< char >( header.end( ) - header.begin( ) ); 
    APESEARCH::copy( header.begin( ), header.end( ), &requester.headerBuff.front( ) );
    requester.resetState( );
    ASSERT_FALSE( requester.gzipped );
    ASSERT_FALSE( requester.chunked );
    ASSERT_FALSE( requester.redirect );
    result = requester.parseHeader( &*requester.headerBuff.end( ) );
    ASSERT_EQUAL( result.status, getReqStatus::successful );
    ASSERT_TRUE( requester.gzipped );
    ASSERT_TRUE( requester.chunked );
    ASSERT_FALSE( requester.contentLength );
    ASSERT_FALSE( requester.redirect );

    //redurected
    header = std::string( 
    "HTTP/1.1 301 Moved Permanently\r\n"
    "Location: http://www.example.org/\r\n"
    "Date: Mon, 22 Feb 2021 12:37:44 GMT\r\n"
    "Expires: -1\r\n"
    "Cache-Control: private, max-age=0\r\n"
    "Content-Type: text/html; charset=ISO-8859-1\r\n"
    "P3P: CP=\"This is not a P3P policy! See g.co/p3phelp for more info.\"\r\n"
    "Server: gws\r\n"
    "X-XSS-Protection: 0\r\n"
    "X-Frame-Options: SAMEORIGIN\r\n"
    "Set-Cookie: 1P_JAR=2021-02-22-12; expires=Wed, 24-Mar-2021 12:37:44 GMT; path=/; domain=.google.com; Secure\r\n"
    "Set-Cookie: NID=209=Q57QitAU2g7nnSs2kS_u970x_z9PtTD7FONQH-4jFMwrBz6Qcttc5l_MAT-b9U32PJQfT5Y8JQ0xxCT4PMvU304OOSk79jEEu9HC7l0kdGWklGQcO3QQkgZ-gvaXlqcCo7VYSdOaVdIUdMEzPpBWMV5oQ20-7_tX6-3yDDZkA90; expires=Tue, 24-Aug-2021 12:37:44 GMT; path=/; domain=.google.com; HttpOnly Alt-Svc: h3-29=\":443\"; ma=2592000,h3-T051=\":443\"; ma=2592000,h3-Q050=\":443\"; ma=2592000,h3-Q046=\":443\"; ma=2592000,h3-Q043=\":443\"; ma=2592000,quic=\":443\"; ma=2592000; v=\"46,43\"\r\n"
    "Accept-Ranges: none\r\n"
    "Vary: Accept-Encoding\r\n"
    "Connection: close\r\n"
    "Content-Length: asbdas\r\n"
    "Content-Length: 1024\r\n"
    "Accept-Ranges: none\r\n\r\n" );

    requester.headerBuff = APESEARCH::vector< char >( header.end( ) - header.begin( ) ); 
    APESEARCH::copy( header.begin( ), header.end( ), &requester.headerBuff.front( ) );
    requester.resetState( );
    ASSERT_FALSE( requester.gzipped );
    ASSERT_FALSE( requester.chunked );
    ASSERT_FALSE( requester.redirect );
    result = requester.parseHeader( &*requester.headerBuff.end( ) );
    ASSERT_EQUAL( result.status, getReqStatus::redirected );
    ASSERT_FALSE( requester.gzipped );
    ASSERT_FALSE( requester.chunked );
    ASSERT_FALSE( requester.contentLength );
    ASSERT_TRUE( requester.redirect );
    ASSERT_EQUAL( requester.contentLengthBytes, 0u );
    ASSERT_EQUAL( result.url, "http://www.example.org/" );
   }

TEST_MAIN()