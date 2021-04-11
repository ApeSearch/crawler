#include "../include/crawler/Database.h"
#include <stdlib.h>
#include <iostream>

//#include <linux/limits.h>

#define NUM_OF_FILES 256
#define MAX_PATH 1024
#define MAX_INT_LENGTH 100

DBBucket::DBBucket(size_t index){
    static const APESEARCH::string anchor_root = "./tests/anchorFiles/anchorFile";
    static const APESEARCH::string parsed_root = "./tests/parsedFiles/parsedFile";

    char path[MAX_PATH];
    snprintf( path, sizeof( path ), "%s%d%s", anchor_root.cstr(), index, ".txt" );
    anchorFile = APESEARCH::File( path, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
    snprintf( path, sizeof( path ), "%s%d%s", parsed_root.cstr(), index, ".txt" );
    parsedFile = APESEARCH::File( path, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
}


Database::Database()
{
    APESEARCH::string anchor_root = "./tests/anchorFiles/anchorFile";
    APESEARCH::string parsed_root = "./tests/parsedFiles/parsedFile";
    char path[MAX_PATH];
    file_vector.reserve( NUM_OF_FILES );
    for(int i = 0; i < NUM_OF_FILES; i++)
    {   
        try
        {
            file_vector.emplace_back( i );
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

    size_t index = hash(link.URL.cstr());
    index = index % file_vector.size();

    {
        APESEARCH::unique_lock<APESEARCH::mutex> lk( file_vector[index].anchor_lock );
        file_vector[index].anchorFile.write(link.URL.cstr(), link.URL.size());
        file_vector[index].anchorFile.write(newline_char, 1);
        for(size_t i = 0; i < link.anchorText.size(); i++){
            file_vector[index].anchorFile.write(link.anchorText[i].cstr(), link.anchorText[i].size());
            file_vector[index].anchorFile.write(space_char, 1);
        }
        file_vector[index].anchorFile.write(null_char, 1);
    }
}

void Database::addParsedFile(HtmlParser &parsed)        //url, words, titleWords, headingWords, boldWords, base, numParagraphs, numHeadings, numSentences 
{
    static const char* const null_char = "\0";
    static const char* const newline_char = "\n";
    static const char* const space_char = " ";
    char temp[MAX_INT_LENGTH];

    size_t index = hash(parsed.url.cstr());
    index = index % file_vector.size();

    APESEARCH::unique_lock<APESEARCH::mutex> lk( file_vector[index].parsed_lock );
    file_vector[index].parsedFile. write(parsed.url.cstr(), parsed.url.size());
    file_vector[index].parsedFile.write(newline_char, 1);
    for(size_t i = 0; i < parsed.words.size(); i++){
        file_vector[index].parsedFile.write(parsed.words[i].cstr(), parsed.words[i].size());
        file_vector[index].parsedFile.write(space_char, 1);
    }
    file_vector[index].parsedFile.write(newline_char, 1);
    for(size_t i = 0; i < parsed.titleWords.size(); i++){
        file_vector[index].parsedFile.write(parsed.titleWords[i].cstr(), parsed.titleWords[i].size());
        file_vector[index].parsedFile.write(space_char, 1);
    }
    file_vector[index].parsedFile.write(newline_char, 1);
    for(size_t i = 0; i < parsed.headingWords.size(); i++){
        file_vector[index].parsedFile.write(parsed.headingWords[i].cstr(), parsed.headingWords[i].size());
        file_vector[index].parsedFile.write(space_char, 1);
    }
    file_vector[index].parsedFile.write(newline_char, 1);
    for(size_t i = 0; i < parsed.boldWords.size(); i++){
        file_vector[index].parsedFile.write(parsed.boldWords[i].cstr(), parsed.boldWords[i].size());
        file_vector[index].parsedFile.write(space_char, 1);
    }
    file_vector[index].parsedFile.write(newline_char, 1);
    file_vector[index].parsedFile.write(parsed.base.cstr(), parsed.base.size());
    file_vector[index].parsedFile.write(newline_char, 1);
    file_vector[index].parsedFile.write(parsed.base.cstr(), parsed.base.size());
    file_vector[index].parsedFile.write(newline_char, 1);

    size_t size = snprintf( temp, sizeof( temp ), "%d", parsed.numParagraphs);
    file_vector[index].parsedFile.write(temp, size);
    file_vector[index].parsedFile.write(newline_char, 1);

    size = snprintf( temp, sizeof( temp ), "%d", parsed.numHeadings);
    file_vector[index].parsedFile.write(temp, size);
    file_vector[index].parsedFile.write(newline_char, 1);

    size = snprintf( temp, sizeof( temp ), "%d", parsed.numSentences);
    file_vector[index].parsedFile.write(temp, size);
    file_vector[index].parsedFile.write(newline_char, 1);

    file_vector[index].parsedFile.write(null_char, 1);
}