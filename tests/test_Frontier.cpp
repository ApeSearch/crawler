#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Frontier.h"
#include <iostream>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h> // for linkat
#include <unistd.h> // for getcwd
#include <iostream>
#include <sys/stat.h> // for mkdir
#include "../libraries/AS/include/AS/File.h"
#include <ftw.h> // for nftw()
#include <stdio.h> // for perror
#include <thread>

static int unlink_cb( const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf )
   {
    int rv = remove( fpath );
    
    if ( rv )
       perror( fpath );
    
    return rv;
   }

static int rmrf( char *path )
   {
    return nftw( path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS );
   }

void insert( UrlFrontier& frontier, const APESEARCH::string& url )
   {
   std::this_thread::sleep_for(std::chrono::seconds{1});
   ParsedUrl parsedUrl( url.cstr() );
   bool ret = frontier.backEnd.insertTiming( std::chrono::time_point_cast<std::chrono::seconds>
   ( std::chrono::system_clock::now() ), std::string( parsedUrl.Host, parsedUrl.Port ) );
   ASSERT_TRUE( ret );
   }

void insertFail( UrlFrontier& frontier, const APESEARCH::string& url )
   {
   std::this_thread::sleep_for(std::chrono::seconds{1});
   ParsedUrl parsedUrl( url.cstr() );
   bool ret = frontier.backEnd.insertTiming( std::chrono::time_point_cast<std::chrono::seconds>
   ( std::chrono::system_clock::now() ), std::string( parsedUrl.Host, parsedUrl.Port ) );
   ASSERT_FALSE( ret );
   }

TEST( test_frontier )
    {
    APESEARCH::PThreadPool< std::deque<APESEARCH::Func> > pool( 1u );
    UrlFrontier frontier( "/tests/DummyFrontier", 1 );
    frontier.insertNewUrl( "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    auto tOf = newTime();
    APESEARCH::string url( frontier.getNextUrl() );
    ASSERT_EQUAL( url, "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle1.html" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle2.html" );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle1.html" );
    APESEARCH::string str( "http://www.example.com/dogs/poodles/poodle1.html" );
    auto futureObj = pool.submit( insert, std::ref( frontier ), std::ref( str ) );
    futureObj.get( );
    url = frontier.getNextUrl( );
    futureObj = pool.submit( insertFail, std::ref( frontier ), std::ref( str ) );
    frontier.insertNewUrl( "https://en.wikipedia.org/wiki/Peer-to-peer" );
    futureObj.get( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle2.html" );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "https://en.wikipedia.org/wiki/Peer-to-peer" );
    }

TEST( test_frontier_NoName )
    {
    APESEARCH::PThreadPool< std::deque<APESEARCH::Func> > pool( 1u );
    UrlFrontier frontier( 1 );
    frontier.insertNewUrl( "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    auto tOf = newTime();
    APESEARCH::string url( frontier.getNextUrl() );
    ASSERT_EQUAL( url, "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle1.html" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle2.html" );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle1.html" );
    APESEARCH::string str( "http://www.example.com/dogs/poodles/poodle1.html" );
    auto futureObj = pool.submit( insert, std::ref( frontier ), std::ref( str ) );
    futureObj.get( );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle2.html" );
    frontier.insertNewUrl( "https://www.youtube.com/watch?v=VYsrx6gZ1As" );
    }

TEST( test_frontier_NoName_After )
   {
   UrlFrontier frontier( 1 );
   APESEARCH::string url( frontier.getNextUrl() );
   ASSERT_EQUAL( url, "https://www.youtube.com/watch?v=VYsrx6gZ1As" );
   }

TEST_MAIN()