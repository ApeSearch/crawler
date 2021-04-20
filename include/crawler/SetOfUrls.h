#pragma once

#ifndef SETOFURLS_H_APE_SEACH
#define SETOFURLS_H_APE_SEACH

#include "../../libraries/AS/include/AS/unique_mmap.h"
#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/File.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/condition_variable.h"
#include <sys/types.h>
#include <dirent.h> // for DIR 
#include <atomic>

struct UrlObj
{
    //struct PriorityFields
    //{
    //size_t num;
    //bool seenInBloomFilter;
    //};

    APESEARCH::string url;
    size_t priority; // Indicate which bucket to place priority
}; // end UrlObj

class SetOfUrls
   {
#ifdef DEBUG
public:
#endif
    static constexpr const char *frontierLoc = "/VirtualFileSystem/Root/Frontier";
    static constexpr size_t maxUrls = 65536;
    unique_mmap frontOfQueue;
    // A specific dirEntry ( what is returned when reading dirent )
    //APESEARCH::vector<char> cwd;
    char cwd[PATH_MAX];
    char dirPath[PATH_MAX];
    char backQPath[PATH_MAX];
    char backFileName[PATH_MAX];
    char frontQFileName[PATH_MAX];
    char const *frontQPtr, *frontQEnd;
    DIR *dir;
    std::atomic<unsigned> numOfUrlsInserted;
    APESEARCH::File back; // protected by backQLk
    
    // Mutex:
    APESEARCH::mutex queueLk; // Needed for dealing with backAnd front
    APESEARCH::mutex dirLk;
    APESEARCH::mutex frntQLk;
    APESEARCH::mutex backQLk;
    APESEARCH::condition_variable cv;
    //APESEARCH::condition_variable priorityCV;
    //std::atomic< bool > highPriorityThreadWaiting;
    std::atomic<size_t> numOfFiles;
    std::atomic< bool >liveliness;

    void startNewFile();
    size_t numOfValidFiles( );
    bool removeFile( const char * );
    APESEARCH::vector<char> getNextDirEntry( DIR *dir );
    bool popNewBatch();
    void finalizeSection( );
    bool verifyFile( const char * ) const;
    bool forceWrite();
    inline UrlObj helperDeq();
    public:
    void shutdown( )
        {
        liveliness.store( false );
        cv.notify_one( );
        } // end shutdown( )
        SetOfUrls();
        SetOfUrls( const char * );
        ~SetOfUrls();
        UrlObj dequeue();
        UrlObj blockingDequeue();
        const char *front();
        void enqueue( const APESEARCH::string &url );
   }; // SetOfUrls

#endif