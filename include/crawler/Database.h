#pragma once

#ifndef DATABASE_H_APESEARCH
#define DATABASE_H_APESEARCH

#include "../../libraries/AS/include/AS/File.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/vector.h"
#include "../../Parser/HtmlParser.h"
#include "../../libraries/HashTable/include/HashTable/FNV.h"
#ifdef LINUX
#include <linux/limits.h>
#endif
#include "../../libraries/AS/include/AS/unique_mmap.h"
#include <unordered_map> //pls dont look at this hamilton
#include <string>
#include <algorithm>
#include <cctype> 
#include <dirent.h> // for DIR 

int verifyWords(char const *parsedPtr);
void reduceFile(const std::string &path);
void formatFile(APESEARCH::File &file);

struct DBBucket
   {
   char directory[1024];
   DBBucket();
   DBBucket( size_t index, const char * dir );
   ~DBBucket() {}
   DBBucket( DBBucket&& other ) : parsedFile( std::move( other.parsedFile ) ), anchorFile( std::move( other.parsedFile ) ) {}
   

   DBBucket& operator=( DBBucket&& other )
      {
      APESEARCH::swap( parsedFile, other.parsedFile );
      APESEARCH::swap( anchorFile, other.anchorFile );
      return *this;
      }

   APESEARCH::mutex anchor_lock;
   APESEARCH::mutex parsed_lock;
   APESEARCH::File parsedFile;
   APESEARCH::File anchorFile;
   };

class Database 
{
#ifdef DEBUG
    public:
#endif
    APESEARCH::vector<DBBucket> file_vector;
    public:
        Database( );
        Database( const char *directory );
        ~Database();
        void addAnchorFile(const Link &link);
        void addParsedFile(const HtmlParser &parser);
        void condenseFile(APESEARCH::File &anchorFile, APESEARCH::File &parsedFile, int index);
        void condenseFiles();
        void parseAnchorFile(char const *anchorPtr, size_t fileSize, std::unordered_map<std::string, int> &anchorMap);
        void cleanAnchorMap(int fileCount);
        void cleanAnchorMap();
        void fillAnchorMap(std::unordered_map<std::string, int> &anchorMap, const char *parsedPtr , int fileSize, int &fileCount);
        FNV hash;
};

#endif
