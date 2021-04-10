#pragma once

#ifndef DATABASE_H_APESEARCH
#define DATABASE_H_APESEARCH

#include "../../libraries/AS/include/AS/File.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/vector.h"
#include "../../Parser/HtmlParser.h"
#include "../../libraries/HashTable/include/HashTable/FNV.h"

class Database 
{
    public:
        Database();
        ~Database();
        void addAnchorFile(Link &link);
        void addParsedFile();
    private:
        APESEARCH::vector<APESEARCH::File> parsedFiles;
        APESEARCH::vector<APESEARCH::File> anchorFiles;
        APESEARCH::vector<APESEARCH::mutex> database_locks;
        FNV hash;
};

#endif