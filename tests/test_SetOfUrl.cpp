
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/SetOfUrls.h"
#include <iostream>

TEST( test_SetOfUrls )
   {
   try
      {
      SetOfUrls set( "tests/input" );

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
      //ASSERT_EQUAL( obj.url, 
      }
   catch ( std::runtime_error& e )
      {
      std::cout << e.what() << std::endl;
      ASSERT_TRUE( false );
      }
   catch( ... )
      {
      ASSERT_TRUE( false );
      }
   }

TEST_MAIN()