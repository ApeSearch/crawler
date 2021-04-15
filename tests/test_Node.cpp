#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Node.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/Socket.h"
#include "../libraries/AS/include/AS/vector.h"
#include "../libraries/AS/include/AS/pthread_pool.h"
#include "../libraries/AS/include/AS/circular_buffer.h"
#include <assert.h>
#include <iostream>
#include <thread>

void func(Node &node, APESEARCH::string &str)
{
    Link link;
    link.URL = str;
    link.anchorText.push_back("word1");
    link.anchorText.push_back("word2");
    link.anchorText.push_back("word3");
    node.write(link);
}

TEST(start_up)
{
    
    Database db;
    UrlFrontier frontier( 1 );
    APESEARCH::vector<APESEARCH::string> ips = {"107.21.75.131", "54.158.7.33"};


    Node node(ips, 0, frontier, db);
    //const char * const exampleUrl = "https://umich.edu/";
    //Call threadpool for Receivers
    APESEARCH::string str1("amazon.com/something");
    APESEARCH::string str2("google.com/something");
    for (size_t i = 0; i < 8; i++)
    {
        std::thread t1(func, std::ref(node), std::ref(str1));
        std::thread t2(func, std::ref(node), std::ref(str2));
        t1.detach();
        t2.detach();
    }

    sleep(300u);
}


TEST_MAIN()
