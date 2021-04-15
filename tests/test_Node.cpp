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
    APESEARCH::vector<APESEARCH::string> ips = {"52.207.241.143", "54.226.70.168"};


    Node node(ips, 0, frontier, db);
    APESEARCH::vector<APESEARCH::string> vec = {"amazon.com/something", "google.com/something", "umich.com/something", "youtube.com/something"};
    for (size_t i = 0; i < vec.size(); i++)
    {
        std::cout << vec[i] << "is in Node: " << (db.hash(vec[i].cstr()) & 1) << " and in anchor file: " << db.hash(vec[i].cstr()) % 256 << '\n';
    }

    for (size_t i = 0; i < 4; i++)
    {
        for ( size_t n = 0; n <  vec.size(); ++n )
            {
            std::thread t = std::thread( func, std::ref( node ), std::ref( vec[i] ) );
            t.detach();
            }
    }
    sleep(300u);
}


TEST_MAIN()
