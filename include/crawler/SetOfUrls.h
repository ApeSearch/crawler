

#pragma once

#ifndef SETOFURLS_H_APE_SEACH
#define SETOFURLS_H_APE_SEACH

#include "../../libraries/AS/include/AS/unique_mmap.h"
#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/File.h"
#include "../../libraries/AS/include/AS/string.h"
#include <sys/types.h>
#include <dirent.h> // for DIR 

struct UrlObj
{
    //struct PriorityFields
    //{
    //size_t num;
    //bool seenInBloomFilter;
    //};

    APESEARCH::string url;
    size_t priority; // Indicate which bucket to place priority
};

class SetOfUrls
   {
#ifdef DEBUG
public:
#endif
    static constexpr const char *frontierLoc = "VirtualFileSystem/Root/Frontier";
    static constexpr size_t maxUrls = 8;
    unique_mmap frontOfQueue;
    // A specific dirEntry ( what is returned when reading dirent )
    //APESEARCH::vector<char> cwd;
    char cwd[PATH_MAX];
    size_t cwdLength;
    DIR *dir;
    char *frontQPtr, *backQPtr, *frontQEnd;
    unsigned numOfUrlsInserted;
    APESEARCH::File back;

    void startNewFile();
    bool popNewBatch();
    void finalizeSection( );

    public:
        SetOfUrls();
        SetOfUrls( const char * );
        ~SetOfUrls();
        UrlObj dequeue();
        void enqueue( const APESEARCH::string &url );
   }; // SetOfUrls

#endif