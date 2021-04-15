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

void func(Node &node)
{
    Link link;
    link.URL = "amazon.com/something";
    link.anchorText.push_back("word1");
    link.anchorText.push_back("word2");
    link.anchorText.push_back("word3");
    node.write(link);
}

TEST(start_up)
{
    
    Database db;
    UrlFrontier frontier( 1 );
    APESEARCH::vector<APESEARCH::string> ips = {"127.0.0.1", "192.168.1.100"};

    int num = db.hash("google.com/something") % db.file_vector.size();

    Node node(ips, 1, frontier, db);
    //const char * const exampleUrl = "https://umich.edu/";
    //Call threadpool for Receivers
    
    for (size_t i = 0; i < 256; i++)
    {
        std::thread t(func, std::ref(node));
        t.detach();
    }
    sleep(5u);
    sleep(300u);
}


TEST_MAIN()