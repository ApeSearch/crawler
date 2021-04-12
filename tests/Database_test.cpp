
#include "../libraries/unit_test_framework/include/unit_test_framework/unit_test_framework.h"
#include "../include/crawler/Database.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include <iostream>

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


TEST_MAIN()