#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Node.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/Socket.h"
#include "../libraries/AS/include/AS/vector.h"
#include "../libraries/AS/include/AS/pthread_pool.h"
#include "../libraries/AS/include/AS/circular_buffer.h"
#include "../libraries/AS/include/AS/mutex.h"
#include <assert.h>
#include <iostream>
#include <thread>

APESEARCH::mutex coutLk;

//void func(Node &node, APESEARCH::string &str)
//{
    //Link link;
    //link.URL = str;
    //link.anchorText.push_back("word1");
    //link.anchorText.push_back("word2");
    //link.anchorText.push_back("word3");
    //node.write(link);
//}
void func(UrlFrontier &frontier, APESEARCH::string &str)
{
    Link link;
    link.URL = str;
    link.anchorText.push_back("word1");
    link.anchorText.push_back("word2");
    link.anchorText.push_back("word3");
    for ( unsigned n = 0; n < 5; ++n )
       {
        frontier.insertNewUrl( link.URL );
        APESEARCH::unique_lock<APESEARCH::mutex> lk( coutLk );
        std::cout << "Inserted: " << str << std::endl;
       }
}

TEST(start_up)
{
    Database db;
    UrlFrontier frontier( 1 );
    //Christians,Alexs,Pauls_first, Pauls_second
    APESEARCH::vector<APESEARCH::string> ips = {"3.131.182.8","34.201.187.203","23.21.84.212","34.233.155.58"};
    Node node(ips, 0, frontier, db);
    APESEARCH::vector<APESEARCH::string> vec = {"https://www.yahoo.com/something", "https://www.youtube.com/something", "https://www.gmail.com/something", "https://www.reddit.com/something", "https://www.blue.com/something"};
    for (size_t i = 0; i < vec.size(); i++)
    {
       std::cout << vec[i] << "is in Node: " << (db.hash(vec[i].cstr()) & 1) << " and in anchor file: " << db.hash(vec[i].cstr()) % 256 << '\n';
    }

    //for (size_t i = 0; i < 5; i++)
    //{
        for ( size_t n = 0; n < vec.size( ); ++n )
            {
            std::thread t = std::thread( func, std::ref( frontier ), std::ref( vec[n] ) );
            t.detach();
            }
    //}
    //sleep(5u);
    //youtube.com/somethingis in Node: 1 and in anchor file: 39
    //google.com/somethingis in Node: 1 and in anchor file: 225
    //youtube.com/somethingis in Node: 1 and in anchor file: 39

    for ( unsigned n = 0; n < 25; ++n )
        {
        coutLk.lock( );
        std::cout << "Getting next url:\n";
        coutLk.unlock( );
        APESEARCH::string str( frontier.getNextUrl( ) );
        APESEARCH::unique_lock<APESEARCH::mutex> lk( coutLk );
        std::cout << "Got: ";
        std::cout << str << std::endl;
        ParsedUrl parsedUrl( str.cstr( ) );
        std::string domain( parsedUrl.Host, parsedUrl.Port );
        std::cout << "\nInserting domain: " << domain << std::endl;
        std::cout << "Amount of characters: " << domain.size( ) << std::endl;
        std::cout << "Difference: " << parsedUrl.Port - parsedUrl.Host << std::endl;
        lk.unlock( );
        frontier.initiateInsertToDomain( 
            std::chrono::time_point_cast<std::chrono::seconds>
                ( std::chrono::system_clock::now() ), std::move( domain ) );
        } // end for

    std::cout << "Finish checking..." << std::endl;;
    exit( 0 );
    sleep(300u);

}


TEST_MAIN()
