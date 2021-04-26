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
    input.URL = "https://www.google.com";
    input.anchorText = {"1", "2", "3", "4"};
    size_t ind = db.hash( input.URL.cstr() ) % db.file_vector.size();
    db.file_vector[ind].anchorFile.truncate(0);
    db.addAnchorFile(input);
    APESEARCH::string check = "https://www.google.com";
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
    input.URL = "https://www.google.com";
    APESEARCH::string check = "https://www.google.com";
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
        parser.url = "https://www.google.com";
        APESEARCH::vector<Word> temp;
        for(int i = 0; i < 10; i++){
            temp.emplace_back("monke", static_cast<Type>(i % 4));
        }
        parser.parsed_text = temp;
        parser.base = "https://www.monkey.com";
        parser.numHeadings = 420;
        parser.numParagraphs = 666;
        parser.numSentences = 440;
        db.addParsedFile(parser);
        APESEARCH::string check = "https://www.google.com\nmonke monke monke monke monke monke monke monke monke monke \n1 5 9 \n2 6 \n3 7 \nhttps://www.monkey.com\n666\n420\n440\n";
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

        HtmlParser parser(ptr, file.fileSize(), "https://www.monkey.com" );
        Database db;
        size_t ind = db.hash( parser.url.cstr() ) % db.file_vector.size();
        std::cout << ind << std::endl;
        db.addParsedFile(parser);

        path = "./Parser/inputs/gamefaqs.html";
        APESEARCH::File file1(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map1( file1.fileSize(), PROT_WRITE, MAP_SHARED, file1.getFD(), 0 );
        const char *ptr1 = reinterpret_cast< const char *>( map1.get() );

        HtmlParser parser1(ptr1, file1.fileSize(), "https://www.monkey.com" );
        // size_t ind1 = db.hash( parser1.url.cstr() ) % db.file_vector.size();
        // std::cout << ind1 << std::endl;
        db.addParsedFile(parser1);

        path = "./Parser/inputs/wiki.html";
        APESEARCH::File file2(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map2( file2.fileSize(), PROT_WRITE, MAP_SHARED, file2.getFD(), 0 );
        const char *ptr2 = reinterpret_cast< const char *>( map2.get() );

        HtmlParser parser2(ptr2, file2.fileSize(), "https://www.monkey.com" );
        db.addParsedFile(parser2);

        path = "./Parser/inputs/yoast.html";
        APESEARCH::File file3(path.cstr(), O_RDWR , mode_t(0600));
        unique_mmap map3( file3.fileSize(), PROT_WRITE, MAP_SHARED, file3.getFD(), 0 );
        const char *ptr3 = reinterpret_cast< const char *>( map3.get() );

        HtmlParser parser3(ptr3, file3.fileSize(), "https://www.monkey.com" );
        db.addParsedFile(parser3);
    }
TEST(test_parsed_anchor_file){
    std::string check = "https://www.google.com";
    check += "\n";
    check += "1 2 3 4 ";
    check += "\n";
    check.push_back('\0');
    check += "https://www.monkey.com";
    check += "\n";
    check += "monkey good ";
    check += "\n";
    check.push_back('\0'); 
    std::unordered_map<std::string, int> map;
    map["https://www.google.com"] = 0;

    Database db;
    int fileCount = 0;
    db.parseAnchorFile(check.c_str(), check.length(), map);
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
    std::string anchorString = "https://www.monkey.com\nhello \n";
    anchorString.push_back('\0');
    anchorString += "https://www.ape.com\nfuck \n";
    anchorString.push_back('\0');
    anchorString += "https://www.monkey.com\nmonkey monkey \n";
    anchorString.push_back('\0');
    anchorString += "https://www.monkey.com\nmonkey monkey \n";
    anchorString.push_back('\0');
    anchorString += "https://www.bonobo.com\npoop \n";
    anchorString.push_back('\0');

    std::string parsedString = "https://www.monkey.com\nmonkeys are strong \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "https://www.ape.com\napes are strong \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    anchor.write(anchorString.c_str(), anchorString.length());
    parsed.write(parsedString.c_str(), parsedString.length());

    std::cout << anchor.fileSize() << " " << parsed.fileSize() << std::endl;

    Database db;
    db.condenseFile(anchor, parsed, 0);
    std::string condPath = "./condensedFiles/condensedFile0";
    APESEARCH::File condensed(condPath.c_str(), O_RDWR, mode_t(0600));
    unique_mmap mmap( condensed.fileSize(), PROT_READ, MAP_SHARED, condensed.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    APESEARCH::string readData( ptr, ptr + condensed.fileSize() );
    APESEARCH::string check = "https://www.monkey.com\nmonkeys are strong \n0 2 \n\n1 \n\n2\n3\n4\n\"monkey monkey\" 2\n\"hello\" 1\n";
    check.push_back('\0');
    check += "https://www.ape.com\napes are strong \n0 2 \n\n1 \n\n2\n3\n4\n\"fuck\" 1\n";
    check.push_back('\0');
    ASSERT_EQUAL(check, readData);
    condensed.truncate(0);
    anchor.truncate(0);
    parsed.truncate(0);

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

TEST(test_condense_files_empty){
    Database db;
    db.condenseFiles();
}

TEST(test_parsed_anchor_file_broken){
    std::string check = "https://www.google.com";
    check += "\n";
    check += "1 2 3 4 \n";
    check += "\n";                  //this is broken format extra newline 
    check.push_back('\0');
    check += "https://www.monkey.com\n";
    check += "\n";
    check += "monkey good";
    check += "\n";
    check.push_back('\0');
    check += "https://www.google.com\n1 2 3 4 \n";
    check.push_back('\0');
    std::unordered_map<std::string, int> map;
    map["https://www.google.com"] = 0;

    Database db;
    int fileCount = 0;
    db.parseAnchorFile(check.c_str(), check.length(), map);
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

TEST(test_condense_broken_parsed){
    std::string path1 = "../testFiles/anchorTest";
    std::string path2 ="../testFiles/parsedTest";
    APESEARCH::File anchor(path1.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    APESEARCH::File parsed(path2.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    std::string anchorString = "https://www.monkey.com\nhello \n";
    anchorString.push_back('\0');
    anchorString += "https://www.ape.com\nfuck \n";
    anchorString.push_back('\0');
    anchorString += "http://www.monkey.com\nmonkey monkey \n";
    anchorString.push_back('\0');
    anchorString += "https://www.monkey.com\nmonkey monkey \n";
    anchorString.push_back('\0');
    anchorString += "http://www.bonobo.com\npoop \n";
    anchorString.push_back('\0');

    std::string parsedString = "https://www.monkey.com\n12 54 67 8 \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "https://www.ape.com\napes !@ %^# << \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "http://www.bonobo.com\n\n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    anchor.write(anchorString.c_str(), anchorString.length());
    parsed.write(parsedString.c_str(), parsedString.length());

    Database db;
    db.condenseFile(anchor, parsed, 0);
    std::string condPath = "./condensedFiles/condensedFile0";
    APESEARCH::File condensed(condPath.c_str(), O_RDWR, mode_t(0600));
    ASSERT_EQUAL(condensed.fileSize(), 0);
    condensed.truncate(0);
    anchor.truncate(0);
    parsed.truncate(0);
}

TEST(test_condense_broken_anchor){
    std::string path1 = "../testFiles/anchorTest";
    std::string path2 ="../testFiles/parsedTest";
    APESEARCH::File anchor(path1.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    APESEARCH::File parsed(path2.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    std::string anchorString = "https://www.monkey.com\n23 \n";
    anchorString.push_back('\0');
    anchorString += "https://www.monkey.com\nmonkey!!!! \n";
    anchorString.push_back('\0');

    std::string parsedString = "https://www.monkey.com\nyes no \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "https://www.ape.com\n,, !! ++ \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "http://www.bonobo.com\n\n\n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    anchor.write(anchorString.c_str(), anchorString.length());
    parsed.write(parsedString.c_str(), parsedString.length());

    Database db;
    db.condenseFile(anchor, parsed, 0);
    std::string condPath = "./condensedFiles/condensedFile0";
    APESEARCH::File condensed(condPath.c_str(), O_RDWR, mode_t(0600));
    unique_mmap mmap( condensed.fileSize(), PROT_READ, MAP_SHARED, condensed.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    std::string readData( ptr, ptr + condensed.fileSize() );
    std::string check = "https://www.monkey.com\nyes no \n0 2 \n\n1 \n\n2\n3\n4\n\"23\" 1\n";
    check.push_back('\0');

    ASSERT_EQUAL(readData, check);
    condensed.truncate(0);
    anchor.truncate(0);
    parsed.truncate(0);

}

TEST(test_home_depot){
    std::string path1 = "../testFiles/anchorTest";
    std::string path2 ="../testFiles/parsedTest";
    APESEARCH::File anchor(path1.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    APESEARCH::File parsed(path2.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    std::string anchorString = "";

    std::string parsedString = "https://www.homedepot.com#departments\nthe home depot 1 home improvement retailer store finder truck amp tool rental for the pro gift cards credit services track order track order help to see inventory choose a store delivering to cancel my account lists welcome back sign in register orders amp purchases my home depot credit card account profile instant checkout settings lists my subscriptions purchase history cards amp accounts account profile lists pro xtra perks my account switch accounts sign out <container class recommendations contents >< container> sign in manage your account < defs> accounticons <path class cls 1 d m14 71 8 84a5 54 5 54 0 0 0 1 83 31 8 56 8 56 0 0 0 1 6 73a4 65 4 65 0 0 0 8 61 0 4 65 4 65 0 0 0 5 93 8 42 8 7 8 7 0 0 0 0 16 69h9 26a5 81 5 81 0 1 0 5 45 7 85zm5 51 4 65a3 1 3 1 0 1 1 3 1 3 13a3 12 3 12 0 0 1 5 51 4 65zm1 66 15 17a7 14 7 14 0 0 1 7 5 67 6 77 6 77 0 0 1 2 62 53 5 85 5 85 0 0 0 2 32 4 68c0 16 0 31 0 46zm13 05 3 89a4 35 4 35 0 1 1 19 14 71 4 33 4 33 0 0 1 14 71 19 06z ><polygon class cls 1 points 15 46 13 93 15 46 11 32 13 96 11 32 13 96 13 93 11 39 13 93 11 39 15 44 13 96 15 44 13 96 18 05 15 46 18 05 15 46 15 44 18 03 15 44 18 03 13 93 15 46 13 93 > create an account track orders check out faster and create lists cart 0 items all departments home decor furniture amp kitchenware diy projects amp ideas project calculators installation amp services specials amp offers local ad appliances bath amp faucets blinds amp window treatments building materials decor amp furniture doors amp windows electrical flooring amp area rugs hardware heating amp cooling kitchen amp kitchenware lawn amp garden lighting amp ceiling fans outdoor living amp patio paint plumbing storage amp organization tools my account lists all departments home decor furniture amp kitchenware diy projects amp ideas project calculators installation amp services specials amp offers local ad store finder truck amp tool rental for the pro gift cards credit services track order track order help back all departments appliances bath amp faucets blinds amp window treatments building materials decor amp furniture doors amp windows electrical flooring amp area rugs hardware heating amp cooling kitchen amp kitchenware lawn amp garden lighting amp ceiling fans outdoor living amp patio paint plumbing storage amp organization tools back back sunny days greener lawns shop lawn amp garden bring on cordless power free delivery on select outdoor tools spring is in the air new styles now in stock up to 40 off select furniture shop by category popular categories area rugs bathroom mirrors blinds ceiling fans ladders lawn mowers led string light patio furniture seesaws shiplap shoe racks vanities < views shared components internal links internallinks partial html > how doers get more done trade need help please call us at 1 800 home depot 1 800 466 3337 customer service check order status check order status pay your credit card order cancellation returns shipping amp delivery product recalls help amp faqs resources specials amp offers diy projects amp ideas truck amp tool rental installation amp services moving supplies amp rentals real estate floor plan services protection plans rebate center gift cards catalog subscriptions about us careers corporate information digital newsroom home depot foundation investor relations government customers suppliers amp providers affiliate program eco options corporate responsibility home depot licensing information special financing available everyday pay amp manage your card credit offers get 5 off when you sign up for emails with savings and tips please enter in your email address in the following format you@domain com enter email address go shop our brands copy 2000 2021 home depot product authority llc all rights reserved use of this site is subject to certain terms of use local store prices may vary from those displayed products shown as available are normally stocked but inventory levels cannot be guaranteed for screen reader problems with this website please call 1 800 430 3376 or text 38698 standard carrier rates apply to texts privacy amp security statement cookie usage manage my marketing preferences california privacy rights do not sell my personal information california supply chain act site map store directory mobile site welcome to the home depot in order to ensure that you have a great shopping experience please select from the sites below nbsp nbsp home depot usa nbsp nbsp nbsp homedepot com nbsp nbsp home depot canada nbsp nbsp nbsp english francais\n 0 1 2 88 \n81 82 300 301 302 553 554 555 556 780 781 782 783 784 802 803 804 805 806 810 811 812 813 814 815 816 820 821 \n70\n5\n4\n\n";
    parsedString.push_back('\0');
    anchor.write(anchorString.c_str(), anchorString.length());
    parsed.write(parsedString.c_str(), parsedString.length());

    Database db;
    db.condenseFile(anchor, parsed, 0);
    std::string condPath = "./condensedFiles/condensedFile0";
    APESEARCH::File condensed(condPath.c_str(), O_RDWR, mode_t(0600));
    unique_mmap mmap( condensed.fileSize(), PROT_READ, MAP_SHARED, condensed.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( mmap.get() );
    std::string readData( ptr, ptr + condensed.fileSize() );
    ASSERT_EQUAL(readData, parsedString);
    condensed.truncate(0);
    anchor.truncate(0);
    parsed.truncate(0);
}

TEST(test_clean_anchor_map_initial){
    std::string path ="./anchorMapFiles0/anchorMapFile0";
    APESEARCH::File anchorMap(path.c_str(), O_RDWR | O_CREAT  , mode_t(0600));
    std::string input = "Bad";
    anchorMap.write(input.c_str(), input.length());
    Database db;
    assert(anchorMap.fileSize() == 0);

}

TEST(test_fill_anchor_Map){
    Database db;
    std::string parsedString = "https://www.monkey.com\nyes no \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "https://www.ape.com\n,, !! ++ \n0 2 \n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    parsedString += "http://www.bonobo.com\n\n\n\n1 \n\n2\n3\n4\n";
    parsedString.push_back('\0');
    std::unordered_map<std::string, int> map;
    int fileCount = 0;
    db.fillAnchorMap(map, parsedString.c_str(), parsedString.length(), fileCount);
    assert(map.find("https://www.monkey.com") != map.end());
    assert(map.find("https://www.notmonkey.com") == map.end());
}

TEST_MAIN()