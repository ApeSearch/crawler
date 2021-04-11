#include "../include/crawler/Database.h"
#include <stdlib.h>
#include <iostream>

//#include <linux/limits.h>

#define NUM_OF_FILES 256
#define MAX_PATH 1024

Database::Database()
{
    APESEARCH::string anchor_root = "./anchorFiles/anchorFile";
    APESEARCH::string parsed_root = "./parsedFiles/parsedFile";
    char path[MAX_PATH];
    for(int i = 0; i < NUM_OF_FILES; i++)
    {   
        try
        {
            snprintf( path, sizeof( path ), "%s%d%s", anchor_root.cstr(), i, ".txt" );
            anchorFiles.emplace_back( path, O_RDWR | O_CREAT | O_EXCL | O_APPEND, (mode_t) 0600 );
            snprintf( path, sizeof( path ), "%s%d%s", parsed_root.cstr(), i, ".txt" );
            parsedFiles.emplace_back( path, O_RDWR | O_CREAT | O_EXCL | O_APPEND , (mode_t) 0600 );
        }
        catch(APESEARCH::File::failure &f)
        {
            std::cerr << f.getErrorNumber();
            exit(1);
        }
    }
}
Database::~Database()
{
}

void Database::addAnchorFile(Link &link){
    if(link.anchorText.empty())
    {
        return;
    }
    static const char* const null_char = "\0";
    static const char* const newline_char = "\n";
    static const char* const space_char = " ";

    int index = hash(link.URL.cstr());
    index = index % anchorFiles.size();

    {
        APESEARCH::unique_lock<APESEARCH::mutex> lk( database_locks[index] );
        anchorFiles[index].write(link.URL.cstr(), link.URL.size());
        anchorFiles[index].write(newline_char, 1);
        for(int i = 0; i < link.anchorText.size(); i++){
            anchorFiles[index].write(link.anchorText[i].cstr(), link.anchorText[i].size());
            anchorFiles[index].write(space_char, 1);
        }
        anchorFiles[index].write(null_char, 1);
    }
}

void Database::addParsedFile()
{
    
}