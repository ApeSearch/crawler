
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>


TEST( test_addAnchorFile )
{
    Database db;
    Link input;
    input.URL = "www.google.com";
    input.anchorText = {"1", "2", "3", "4"};
    size_t ind = db.hash( input.URL.cstr() ) % db.file_vector.size();
    db.file_vector[ind].anchorFile.truncate(0);
    db.addAnchorFile(input);
    APESEARCH::string check = "www.google.com";
    check += "\n";
    check += "1 2 3 4 ";
    check += "\n";
    check.push_back('\0');

    char buf[1024];
    memset( buf, 0, sizeof( buf ) );
    std::cout << ind << std::endl;
    int fd = db.file_vector[ind].anchorFile.getFD();
    size_t fSize = db.file_vector[ind].anchorFile.fileSize();
    ASSERT_TRUE( fSize > 0 );

    unique_mmap map( fSize, PROT_READ, MAP_SHARED, fd, 0 );
    char const *ptr = reinterpret_cast< char const *>( map.get() ); 
    APESEARCH::string readData( ptr, ptr + fSize );
    ASSERT_EQUAL( check, readData );
}

TEST( test_addAnchorFile_ReOpen )
   {
    Database db;
    Link input;
    input.URL = "www.google.com";
    APESEARCH::string check = "www.google.com";
    check += "\n";
    check += "1 2 3 4 ";
    check += "\n";
    check.push_back('\0');
    size_t ind = db.hash( input.URL.cstr() ) % db.file_vector.size();

    char buf[1024];
    memset( buf, 0, sizeof( buf ) );
    std::cout << ind << std::endl;
    int fd = db.file_vector[ind].anchorFile.getFD();
    size_t fSize = db.file_vector[ind].anchorFile.fileSize();
    ASSERT_TRUE( fSize > 0 );

    unique_mmap map( fSize, PROT_READ, MAP_SHARED, fd, 0 );
    char const *ptr = reinterpret_cast< char const *>( map.get() ); 
    APESEARCH::string readData( ptr, ptr + fSize );
    ASSERT_EQUAL( check, readData );
   }

TEST(test_add_parsed_file)
    {
        Database db;
        HtmlParser parser;
        parser.url = "www.google.com";
        APESEARCH::vector<Word> temp;
        for(int i = 0; i < 10; i++){
            temp.emplace_back("monke", static_cast<Type>(i % 4));
        }
        parser.parsed_text = temp;
        parser.base = "monkey.com";
        parser.numHeadings = 420;
        parser.numParagraphs = 666;
        parser.numSentences = 440;
        db.addParsedFile(parser);
        APESEARCH::string check = "www.google.com\nmonke monke monke monke monke monke monke monke monke monke \n1 5 9 \n2 6 \n3 7 \nmonkey.com\n666\n420\n440\n";
        check.push_back('\0');


        size_t ind = db.hash( parser.url.cstr() ) % db.file_vector.size();
        db.file_vector[ind].parsedFile.truncate(0);
        db.addParsedFile(parser);
        char buf[1024];
        memset( buf, 0, sizeof( buf ) );
        std::cout << ind << std::endl;
        int fd = db.file_vector[ind].parsedFile.getFD();
        size_t fSize = db.file_vector[ind].parsedFile.fileSize();
        ASSERT_TRUE( fSize > 0 );

        unique_mmap map( fSize, PROT_READ, MAP_SHARED, fd, 0 );
        char const *ptr = reinterpret_cast< char const *>( map.get() ); 
        APESEARCH::string readData( ptr, ptr + fSize );
        ASSERT_EQUAL( check, readData );

    }
TEST(test_parsed_site)
    {
        APESEARCH::string path = "./Parser/inputs/NYTimes.html";
        APESEARCH::File file(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map( file.fileSize(), PROT_WRITE, MAP_SHARED, file.getFD(), 0 );
        const char *ptr = reinterpret_cast< const char *>( map.get() );

        HtmlParser parser(ptr, file.fileSize(), "www.monkey.com" );
        Database db;
        size_t ind = db.hash( parser.url.cstr() ) % db.file_vector.size();
        std::cout << ind << std::endl;
        db.addParsedFile(parser);

        path = "./Parser/inputs/gamefaqs.html";
        APESEARCH::File file1(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map1( file1.fileSize(), PROT_WRITE, MAP_SHARED, file1.getFD(), 0 );
        const char *ptr1 = reinterpret_cast< const char *>( map1.get() );

        HtmlParser parser1(ptr1, file1.fileSize(), "www.monkey.com" );
        // size_t ind1 = db.hash( parser1.url.cstr() ) % db.file_vector.size();
        // std::cout << ind1 << std::endl;
        db.addParsedFile(parser1);

        path = "./Parser/inputs/wiki.html";
        APESEARCH::File file2(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map2( file2.fileSize(), PROT_WRITE, MAP_SHARED, file2.getFD(), 0 );
        const char *ptr2 = reinterpret_cast< const char *>( map2.get() );

        HtmlParser parser2(ptr2, file2.fileSize(), "www.monkey.com" );
        db.addParsedFile(parser2);

        path = "./Parser/inputs/yoast.html";
        APESEARCH::File file3(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map3( file3.fileSize(), PROT_WRITE, MAP_SHARED, file3.getFD(), 0 );
        const char *ptr3 = reinterpret_cast< const char *>( map3.get() );

        HtmlParser parser3(ptr3, file3.fileSize(), "www.monkey.com" );
        db.addParsedFile(parser3);
    }

    



TEST_MAIN()