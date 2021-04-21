
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include <cstdio>

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
TEST(test_parsed_anchor_file){
    std::string check = "www.google.com";
    check += "\n";
    check += "1 2 3 4 ";
    check += "\n";
    check.push_back('\0');
    check += "www.monkey.com";
    check += "\n";
    check += "monkey good";
    check += "\n";
    check.push_back('\0'); 
    std::unordered_map<std::string, int> map;

    Database db;
    int fileCount = 0;
    db.parseAnchorFile(check.c_str(), check.length(), map, fileCount);
    APESEARCH::string path = "./anchorMapFiles0/anchorMapFile0";
    APESEARCH::File file(path.cstr(), O_RDWR , mode_t(0600));
    unique_mmap mmap( file.fileSize(), PROT_READ, MAP_SHARED, file.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    APESEARCH::string readData( ptr, ptr + file.fileSize() );
    APESEARCH::string temp = "1 2 3 4 ";
    temp.push_back('\n');
    ASSERT_EQUAL( temp, readData );
    db.cleanAnchorMap(2);
}

TEST(test_clean_anchor_map){
    Database db;
    db.cleanAnchorMap(5);
    std::string path = "./anchorMapFiles0/anchorMapFile0";
    APESEARCH::File file(path.c_str(), O_RDWR , mode_t(0600));
    ASSERT_EQUAL( 0, file.fileSize() );
}

TEST(test_reduce_file){
    std::string path = "./tests/anchorFiles/anchorFile0.txt";
    std::string anchorInput = "";
    for(int i = 0; i < 100; i++){
        anchorInput += std::to_string(i % 3);
        anchorInput += " ";
        anchorInput += '\n';
    }
    anchorInput += "1 \n1 \n";
    APESEARCH::File file(path.c_str(), O_RDWR , mode_t(0600));
    file.write(anchorInput.c_str(), anchorInput.length());
    reduceFile(path);
    unique_mmap mmap( file.fileSize(), PROT_READ, MAP_SHARED, file.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    APESEARCH::string readData( ptr, ptr + file.fileSize() );
    APESEARCH::string check = "\"1\" 35\n\"0\" 34\n\"2\" 33\n";
    ASSERT_EQUAL(check, readData);
    file.truncate(0);
}

TEST(test_condense_file){
    std::string path1 = "../testFiles/anchorTest";
    std::string path2 ="../testFiles/parsedTest";
    APESEARCH::File anchor(path1.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    APESEARCH::File parsed(path2.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    std::string anchorString = "www.monkey.com\nhello \n";
    anchorString.push_back('\0');
    anchorString += "www.ape.com\nfuck \n";
    anchorString.push_back('\0');
    anchorString += "www.monkey.com\nmonkey monkey \n";
    anchorString.push_back('\0');
    anchorString += "www.monkey.com\nmonkey monkey \n";

    std::string parsedString = "www.monkey.com\nmonkeys are strong\n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "www.ape.com\napes are strong\n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    anchor.write(anchorString.c_str(), anchorString.length());
    parsed.write(parsedString.c_str(), parsedString.length());

    Database db;
    db.condenseFile(anchor, parsed, 0);
    std::string condPath = "./condensedFiles/condensedFile0";
    APESEARCH::File condensed(condPath.c_str(), O_RDWR, mode_t(0600));
    unique_mmap mmap( condensed.fileSize(), PROT_READ, MAP_SHARED, condensed.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    APESEARCH::string readData( ptr, ptr + condensed.fileSize() );
    APESEARCH::string check = "www.monkey.com\nmonkeys are strong\n0 2 \n\n1 \n\n2\n3\n4\n\"monkey monkey\" 2\n\"hello\" 1\n";
    check.push_back('\0');
    check += "www.ape.com\napes are strong\n0 2 \n\n1 \n\n2\n3\n4\n\"fuck\" 1\n";
    check.push_back('\0');
    ASSERT_EQUAL(check, readData);
    condensed.truncate(0);

}

TEST(test_format_file){
    std::string path = "./anchorFiles/anchorFile0.txt";
    APESEARCH::File anchor(path.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    anchor.truncate(0);
    std::string testString = "monkey ape chimpanzee bonobo \n monkeY Y Y\n";
    testString.push_back('\0');
    testString += " bad     \n \n input input bad input noooooooo";
    anchor.write(testString.c_str(), testString.length());
    formatFile(anchor);
    std::string check = "monkey ape chimpanzee bonobo \n monkeY Y Y\n";
    check.push_back('\0');
    unique_mmap mmap( anchor.fileSize(), PROT_READ, MAP_SHARED, anchor.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    std::string read(ptr, ptr + anchor.fileSize());
    ASSERT_EQUAL(check, read);
    anchor.truncate(0);
    formatFile(anchor);
    std::string read2(ptr, ptr + anchor.fileSize());
    check = "";
    ASSERT_EQUAL(check, read2);
    anchor.truncate(0);
}
TEST_MAIN()