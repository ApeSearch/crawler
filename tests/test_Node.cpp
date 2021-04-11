#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Node.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/vector.h"

#include <assert.h>

TEST(start_up)
{
    Database db = Database();
    SetOfUrls set = SetOfUrls();
    APESEARCH::vector<APESEARCH::string> ips = {"123", "456"};
    APESEARCH::string local = "123";

    Node node(ips, local, set, db);
    //const char * const exampleUrl = "https://umich.edu/";
}


TEST_MAIN()