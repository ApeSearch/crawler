
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/SetOfUrls.h"
#include <iostream>

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
         ASSERT_EQUAL( obj.priority, 69 );
         obj = set.dequeue();
         ASSERT_EQUAL( obj.url, "https://leetcode.com/discuss/interview-question/system-design/124657/Facebook-or-System-Design-or-A-web-crawler-that-will-crawl-Wikipedia" );
         ASSERT_EQUAL( obj.priority, 69 );
         obj = set.dequeue();
         ASSERT_EQUAL( obj.url, "https://www.diffchecker.com/diff" );
         ASSERT_EQUAL( obj.priority, 69 );
         } // end for
      obj = set.dequeue();
      ASSERT_EQUAL( obj.url, "" );
      set.enqueue( "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
      ASSERT_EQUAL( set.numOfUrlsInserted.load(), 1u );
      obj = set.dequeue();
      ASSERT_EQUAL( obj.url, "https://www.youtube.com/watch?v=oHg5SJYRHA0" );
      ASSERT_EQUAL( obj.priority, 69 );
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

TEST_MAIN()