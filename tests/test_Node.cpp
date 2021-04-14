#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Node.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/Socket.h"
#include "../libraries/AS/include/AS/vector.h"

#include <assert.h>
#include <iostream>

TEST(start_up)
{
    Database db;
    UrlFrontier frontier( 1 );
    APESEARCH::vector<APESEARCH::string> ips = {"127.0.0.1", "192.168.1.100"};

    Node node(ips, 1, frontier, db);
    //const char * const exampleUrl = "https://umich.edu/";
    sleep(300u);
}


TEST_MAIN()