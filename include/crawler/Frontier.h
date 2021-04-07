
#pragma once

#ifndef FRONTIER_H_APESEARCH
#define FRONTIER_H_APESEARCH

#include <cstddef> // for std::size_t
#include <unordered_map>
#include <string>
#include <atomic>

#include "../libraries/AS/include/AS/vector.h"
#include "../libraries/AS/include/AS/queue.h"
#include "../libraries/AS/include/AS/circular_buffer.h"
#include "../libraries/AS/include/AS/unique_ptr.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include "../libraries/AS/include/AS/File.h"
#include "ParsedUrl.h"
#include <fstream>
#include <sys/types.h>
#include <dirent.h>

#include<chrono>

struct domainTiming
{
    //Indicates moment when crawlable. Value is computed based on current time
    std::chrono::time_point<std::chrono::system_clock> timeWCanCrawl; 
    APESEARCH::string domain;
    //TODO implement comparator

    struct compareTime
       {
        bool okToReq( const domainTiming& time )
           {
            return true;
           }
       };
};

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
    static constexpr const char *frontierLoc = "VirtualFileSystem/Root/Frontier";
    static constexpr size_t maxUrls = 8;
    APESEARCH::unique_mmap frontOfQueue;
    APESEARCH::unique_mmap backOfQueue;
    // A specific dirEntry ( what is returned when reading dirent )
    APESEARCH::vector<char> cwd;
    DIR *dir;
    char *frontQPtr, *backQPtr;
    unsigned numOfUrlsInserted;
    File back;

    void startNewFile();
    bool popNewBatch();
    void finalizeSection( );

    public:
        SetOfUrls();
        SetOfUrls( const char * );
        ~SetOFUrls();
        UrlObj dequeue();
        void enqueue( const APESEARCH::string &url );
   }; // SetOfUrls


class UrlFrontier
{
    // Representation of the front-end queues
    class FrontEndPrioritizer 
    {
        static const constexpr std::size_t urlsPerPriority = 1024;
        APESEARCH::vector<APESEARCH::queue<UrlObj, 
                APESEARCH::circular_buffer< UrlObj, 
                APESEARCH::DEFAULT::defaultBuffer< UrlObj, urlsPerPriority>
                                        pQueues;
        char *urlsToCrawl_front; // memory-mapped queue of urls to be crawled

        std::size_t pickQueue(); // user-defined prirority for picking which queue to pop from
    public:
        FrontEndPrioritizer() = default;
        UrlObj getUrl();
        void putUrl();
    };

    class BackendPolitenessPolicy
    {
        static constexpr std::size_t endQueueSize = 100;
        static constexpr std::size_t amtEndQueues = 3000; // This assumes 1000 crawlers
        APESEARCH::priority_queue<domainTiming> backendHeap;
        std::unordered_map<APESEARCH::string, size_t> backendDomains;
        std::vector<APESEARCH::queue<UrlObj, 
                APESEARCH::circular_buffer< UrlObj, 
                APESEARCH::DEFAULT::defaultBuffer< UrlObj, endQueueSize>
                                        domainQueues;
        UrlObj obtainRandUrl();
    public:
        BackendPolitenessPolicy( ) = default;
        UrlObj getMostOkayUrl();
    };

    FrontEndPrioritizer frontEnd;
    BackendPolitenessPolicy backEnd;
    SetOfUrls set;
    std::atomic<bool> liveliness;

    UrlFrontier();
    UrlFrontier( const char * );
    
    static constexpr std::size_t frontQueueSize = 1024;
    // "To keep crawling threds busy, 3 times as many backeended queues as crawler threads"
    //class Impl;
    //APESEARCH::unique_ptr<Impl> impl;


    static unsigned ratingOfTopLevelDomain( const char * );

public:
    UrlFrontier() = default;
    void getNextUrl( APESEARCH::string& buffer );
    bool insertNewUrl( APESEARCH::string&& url );


    void shutdown(); // signals threads to stop
};















#endif