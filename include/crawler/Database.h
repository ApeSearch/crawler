#pragma once

#ifndef DATABASE_H_APESEARCH
#define DATABASE_H_APESEARCH

#include "../../libraries/AS/include/AS/File.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/vector.h"
#include "../../Parser/HtmlParser.h"
#include "../../libraries/HashTable/include/HashTable/FNV.h"

struct DBBucket
   {
   DBBucket() = default;
   DBBucket( size_t index );
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
        Database();
        ~Database();
        void addAnchorFile(Link &link);
        void addParsedFile(HtmlParser &parser);
        FNV hash;
};

#endif