
#pragma once

#ifndef FRONTIER_H_APESEARCH
#define FRONTIER_H_APESEARCH

#include <cstddef> // for std::size_t
#include <unordered_map>
#include <string>
#include <atomic>

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/queue.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/circular_buffer.h"
#include "../../libraries/AS/include/AS/unique_mmap.h"
#include "../../libraries/AS/include/AS/File.h"
#include "ParsedUrl.h"
#include "SetOfUrls.h"
#include <sys/types.h>
#include <dirent.h>

#include<chrono>
#include<ctime>

struct domainTiming
{
    //Indicates moment when crawlable. Value is computed based on current time
    std::chrono::time_point<std::chrono::system_clock> timeWCanCrawl; 
    //std::time_t timeWCanCrawl;
    APESEARCH::string domain;

    domainTiming() = default;

    domainTiming( const std::chrono::time_point<std::chrono::system_clock>& _time, APESEARCH::string&& _domain )
       : timeWCanCrawl( _time ), domain( _domain ) {}
    
    //domainTiming( domainTiming&& other ) : timeWCanCrawl( std::move( other.timeWCanCrawl ) ), domain( std::move( other.domain ) ) {}
    domainTiming( const domainTiming& other ) : timeWCanCrawl( other.timeWCanCrawl ), domain( other.domain ) {}

    struct compareTime
       {
        // needs to be greater since we want a min heap
        bool operator()( const domainTiming& lhs, const domainTiming& rhs )
           {
            return lhs.timeWCanCrawl > rhs.timeWCanCrawl;
           }
       };
};


class UrlFrontier
{
    // Representation of the front-end queues
    class FrontEndPrioritizer 
    {
        static const constexpr std::size_t urlsPerPriority = 1024;
        APESEARCH::vector<APESEARCH::queue<UrlObj, APESEARCH::circular_buffer< UrlObj, 
                APESEARCH::DEFAULT::defaultBuffer< UrlObj, urlsPerPriority> > > >
                                        pQueues;

        std::size_t pickQueue(); // user-defined prirority for picking which queue to pop from
    public:
        FrontEndPrioritizer() = default;
        UrlObj getUrl( SetOfUrls& );
        void putUrl();
    };

    class BackendPolitenessPolicy
    {
        static constexpr std::size_t endQueueSize = 100;
        static constexpr std::size_t amtEndQueues = 3000; // This assumes 1000 crawlers
        using DefaultBuf = APESEARCH::circular_buffer< UrlObj, 
                APESEARCH::DEFAULT::defaultBuffer< UrlObj, endQueueSize> >;
        APESEARCH::priority_queue<domainTiming, 
            APESEARCH::BinaryPQ<domainTiming, domainTiming::compareTime >, 
            domainTiming::compareTime> backendHeap;
        APESEARCH::mutex pqLk;

        std::unordered_map<std::string, size_t> backendDomains;
        APESEARCH::mutex mapLk;
        APESEARCH::vector< APESEARCH::queue<UrlObj, DefaultBuf> > domainQueues;
        UrlObj obtainRandUrl();
    public:
        BackendPolitenessPolicy( ) = default;
        UrlObj getMostOkayUrl();
        bool insertTiming( domainTiming&& timing  );
    };

    SetOfUrls set;
    std::atomic<bool> liveliness;

    
    static constexpr std::size_t frontQueueSize = 1024;
    // "To keep crawling threds busy, 3 times as many backeended queues as crawler threads"

    static unsigned ratingOfTopLevelDomain( const char * );

public:
    FrontEndPrioritizer frontEnd;
    BackendPolitenessPolicy backEnd;
    UrlFrontier() = default;
    UrlFrontier( const char * );
    APESEARCH::string getNextUrl( );
    bool insertNewUrl( APESEARCH::string&& url );


    void shutdown(); // signals threads to stop
};

#endif