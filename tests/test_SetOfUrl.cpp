
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/SetOfUrls.h"
#include <iostream>
#include <unordered_map>
#include <thread>
#include <atomic>

static char dirPath[PATH_MAX];

// remove any remaining files...
static void setUpInput()
   {
   char path[PATH_MAX];
   getcwd( dirPath, PATH_MAX );
   size_t length = strlen( dirPath );
   snprintf( dirPath + length, sizeof( dirPath ) - length, "%s", "/tests/input" );
   printf("Opening directory: %s\n", dirPath);

   DIR *dir = opendir( dirPath );
   ASSERT_NOT_EQUAL( dir, nullptr );
   struct dirent *dp;
   while ( ( dp = readdir( dir ) ) != NULL )
      {
      if ( strcmp( dp->d_name, "." ) && strcmp( dp->d_name, ".." ) )
         {
         size_t length = strlen( dirPath );
         dirPath[ length++ ] = '/';
         dirPath[ length++ ] = '\0';
         snprintf( path, sizeof( path ), "%s%s", dirPath, dp->d_name );
         int retVal = remove( path );
         if ( retVal < 0 )
            {
            perror( "Issue with remove:" );
            ASSERT_FALSE( true );
            }
         }
      } // end while
   
   ASSERT_EQUAL( closedir( dir ), 0 );
   }

TEST( test_SetOfUrls )
   {
   setUpInput();
   try
      {
      SetOfUrls set( "/tests/input" );

      
       for ( unsigned n = 0; n < 2; ++n )
         {
         set.enqueue( "https://www.blog.datahut.co/post/how-to-build-a-web-crawler-from-scratch" );
         set.enqueue( "https://leetcode.com/discuss/interview-question/system-design/124657/Facebook-or-System-Design-or-A-web-crawler-that-will-crawl-Wikipedia" );
         set.enqueue( "https://www.diffchecker.com/diff" );
         APESEARCH::unique_lock<APESEARCH::mutex> lk( set.backQLk );
         set.finalizeSection();
         }


      UrlObj obj;
      for ( unsigned n = 0; n < 2; ++n )
         {
         obj = set.dequeue();
         ASSERT_EQUAL( obj.url, APESEARCH::string("https://www.blog.datahut.co/post/how-to-build-a-web-crawler-from-scratch") );
         ASSERT_EQUAL( obj.priority, 0 );
         obj = set.dequeue();
         ASSERT_EQUAL( obj.url, "https://leetcode.com/discuss/interview-question/system-design/124657/Facebook-or-System-Design-or-A-web-crawler-that-will-crawl-Wikipedia" );
         ASSERT_EQUAL( obj.priority, 0 );
         obj = set.dequeue();
         ASSERT_EQUAL( obj.url, "https://www.diffchecker.com/diff" );
         ASSERT_EQUAL( obj.priority, 0 );
         } // end for
      obj = set.dequeue();
      ASSERT_EQUAL( obj.url, "" );
      set.enqueue( "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
      ASSERT_EQUAL( set.numOfUrlsInserted.load(), 1u );
      obj = set.dequeue();
      ASSERT_EQUAL( obj.url, "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
      ASSERT_EQUAL( obj.priority, 0 );
      }
   catch ( std::runtime_error& e )
      {
      std::cout << e.what() << std::endl;
      ASSERT_TRUE( false );
      }
   catch (const std::exception &exc)
      {
       // catch anything thrown within try block that derives from std::exception
      std::cerr << exc.what() << std::endl;
      ASSERT_TRUE( false );
      }
   catch ( const unique_mmap::failure& unimap )
      {
      std::cerr << unimap.what() << std::endl;
      ASSERT_TRUE( false );
      }
  
   }
   TEST( test_SetOfUrls_After )
      {
      SetOfUrls set( "/tests/input" );
      UrlObj obj( set.dequeue() );
      ASSERT_EQUAL( obj.url, "" );
      }

APESEARCH::mutex coutLk;
APESEARCH::mutex mapLk;
std::atomic<bool> count;
void func(SetOfUrls &set, APESEARCH::vector<APESEARCH::string>& vec, std::unordered_map<std::string, size_t>& map, const size_t rounds )
{
   for ( unsigned i = 0; i < rounds; ++i )
      {
      for ( unsigned n = 0; n < vec.size( ); ++n )
         {
         APESEARCH::unique_lock<APESEARCH::mutex> lk( coutLk );
         std::cout << "Inserted: " <<  vec[ n ]  << std::endl;
         lk.unlock( );
         lk = APESEARCH::unique_lock<APESEARCH::mutex>( mapLk );
         ++map[ std::string(  vec[ n ] .begin(),  vec[ n ] .end() ) ];
         lk.unlock( );
         set.enqueue( vec[ n ] );
         }
      }
}

   TEST(  test_SetOfUrls_After_Concurrent )
      {
      std::unordered_map<std::string, size_t> map;
      SetOfUrls set( "/tests/input" );
      APESEARCH::vector<APESEARCH::string> vec = {"yahoo.com/something", "youtube.com/something", "gmail.com/something", "reddit.com/something", "blue.com/something"};
      size_t rounds = 1000;
      for (size_t i = 0; i < rounds; i++)
         {
            std::thread t = std::thread( func, std::ref( set ), std::ref( vec ), std::ref( map ), rounds );
            t.detach();
         } // end for
      
      for ( unsigned n = 0; n < vec.size( ) * rounds * rounds; ++n )
         {
         UrlObj url = set.blockingDequeue( );
         APESEARCH::unique_lock<APESEARCH::mutex> lk( mapLk );
         ASSERT_TRUE( map[ std::string( url.url.begin(), url.url.end() ) ]-- > 0 );
        } // end for
      for ( std::unordered_map<std::string, size_t>::iterator itr = map.begin(); itr != map.end( ); ++itr )
         ASSERT_EQUAL( itr->second, 0 );
      }

TEST_MAIN()