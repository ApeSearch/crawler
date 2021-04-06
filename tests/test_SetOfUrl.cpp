
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Frontier.h"

TEST( test_SetOfUrls )
   {
    SetOfUrls set( "input/dummyUrls.txt" );
    UrlObj set.dequeue();
   }

TEST_MAIN()