
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"

TEST( test_addAnchorFile )
{
    Database db;
    Link input;
    input.URL = "www.google.com";
    input.anchorText = {"1", "2", "3", "4"};
    db.addAnchorFile(input);
}

TEST_MAIN()