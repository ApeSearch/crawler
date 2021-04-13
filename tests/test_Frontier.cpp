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
   frontier.backEnd.insertTiming( std::chrono::time_point_cast<std::chrono::seconds>
   ( std::chrono::system_clock::now() ), std::string( parsedUrl.Host, parsedUrl.Port ) );
   }

TEST( test_frontier )
    {
    
    UrlFrontier frontier( "/tests/DummyFrontier", 1 );
    frontier.insertNewUrl( "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    auto tOf = newTime();
    APESEARCH::string url( frontier.getNextUrl() );
    ASSERT_EQUAL( url, "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle1.html" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle2.html" );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle1.html" );
    std::thread thread( insert, std::ref( frontier ), "http://www.example.com/dogs/poodles/poodle1.html" );
    thread.detach( );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle2.html" );
    }

TEST( test_frontier_NoName )
    {
    UrlFrontier frontier( 1 );
    frontier.insertNewUrl( "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    auto tOf = newTime();
    APESEARCH::string url( frontier.getNextUrl() );
    ASSERT_EQUAL( url, "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle1.html" );
    frontier.insertNewUrl( "http://www.example.com/dogs/poodles/poodle2.html" );
    url = frontier.getNextUrl( );
    ASSERT_EQUAL( url, "http://www.example.com/dogs/poodles/poodle1.html" );
    std::thread thread( insert, std::ref( frontier ), "http://www.example.com/dogs/poodles/poodle1.html" );
    thread.detach( );
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