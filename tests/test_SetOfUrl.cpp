
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Frontier.h"
#include <iostream>

TEST( test_SetOfUrls )
   {
   try
      {
      SetOfUrls set( "input/dummyUrls.txt" );
      UrlObj obj( set.dequeue() );
      }
   catch ( std::runtime_error& e )
      {
      std::cout << e.what() << std::endl;
      ASSERT_TRUE( false );
      }
   }

TEST_MAIN()