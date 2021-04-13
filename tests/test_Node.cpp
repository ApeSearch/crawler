#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Node.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/vector.h"

#include <assert.h>

TEST(start_up)
{
    Database db;
    SetOfUrls set;
    APESEARCH::vector<APESEARCH::string> ips = {"127.0.0.1", "192.168.1.100"};
    APESEARCH::string local = "127.0.0.1";

    Node node(ips, local, set, db);
    //const char * const exampleUrl = "https://umich.edu/";
}


TEST_MAIN()