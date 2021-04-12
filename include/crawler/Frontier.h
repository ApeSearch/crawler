
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
#include "../../libraries/AS/include/AS/as_semaphore.h"
#include "../../libraries/AS/include/AS/Pthread_pool.h"
#include "ParsedUrl.h"
#include "SetOfUrls.h"
#include <sys/types.h>
#include <dirent.h>

#include<chrono>
#include<ctime>

#define NUMOFFRONTQUEUES 1

struct domainTiming
{
    //Indicates moment when crawlable. Value is computed based on current time
    std::chrono::time_point<std::chrono::system_clock> timeWCanCrawl; 
    unsigned index; // Stores index to queue
    //std::time_t timeWCanCrawl;

    domainTiming() = default;

    domainTiming( const std::chrono::time_point<std::chrono::system_clock>& _time, const unsigned _domain )
        : timeWCanCrawl( _time ), index( _domain ) {}
    
    //domainTiming( domainTiming&& other ) : timeWCanCrawl( std::move( other.timeWCanCrawl ) ), domain( std::move( other.domain ) ) {}
    domainTiming( const domainTiming& other ) : timeWCanCrawl( other.timeWCanCrawl ), index( other.index ) {}

    struct compareTime
       {
        // needs to be greater since we want a min heap
        bool operator()( const domainTiming& lhs, const domainTiming& rhs )
           {
            return lhs.timeWCanCrawl > rhs.timeWCanCrawl;
           }
       };
};

template< size_t _Sizeu >
struct QueueWLock
   {
    APESEARCH::queue<APESEARCH::string, APESEARCH::circular_buffer< APESEARCH::string, 
                APESEARCH::DEFAULT::defaultBuffer< APESEARCH::string, _Sizeu> > > pQueue;
    APESEARCH::mutex queueLk;
    bool isInsertedToHeap = false;
    lockedQueue( ) = default;
   };



class UrlFrontier
{
    // Representation of the front-end queues
    class FrontEndPrioritizer 
    {
    public:
        static const constexpr std::size_t urlsPerPriority = 1024;
        APESEARCH::vector< QueueWLock< urlsPerPriority > > pQueues;
        inline std::size_t pickQueue(); // user-defined prirority for picking which queue to pop from
        UrlObj helperReadInUrl( SetOfUrls& set, std::atomic<bool>& liveliness );
        // Thread that is continously trying to keep QueueWLock full at all times
        void readInUrl( SetOfUrls& set, std::atomic<bool>& );

        APESEARCH::semaphore empty;
        APESEARCH::semaphore full;

        FrontEndPrioritizer( size_t numOfQueues = NUMOFFRONTQUEUES ) : pQueues( numOfQueues ), empty( QueueWLock::urlsPerPriority * numOfQueues ), full( 0 ) {}
        APESEARCH::string getUrl();
    };

    class BackendPolitenessPolicy
    {
    public:
        static constexpr std::size_t endQueueSize = 100;
        struct BackEndQueue
            {
            QueueWLock< endQueueSize > queueWLk;
            APESEARCH::condition_variable queueCV;
            };
        static constexpr std::size_t amtEndQueues = 3000; // This assumes 1000 crawlers
        APESEARCH::priority_queue<domainTiming, 
            APESEARCH::BinaryPQ<domainTiming, domainTiming::compareTime >, 
            domainTiming::compareTime> backendHeap;
        APESEARCH::vector< BackEndQueue > domainQueues;
        std::unordered_map<std::string, size_t> domainsMap;
        APESEARCH::mutex pqLk;
        APESEARCH::mutex mapLk;
        APESEARCH::semaphore semaHeap;
        APESEARCH::condition_variable cvHeap; // for priority queue

        UrlObj obtainRandUrl();

        BackendPolitenessPolicy() = default;
        BackendPolitenessPolicy( const size_t numOfQueues );
        void fillUpEmptyBackQueue( const size_t);
        domainTiming getMostOkayUrl( SetOfUrls& );
        bool insertTiming( const std::chrono::time_point<std::chrono::system_clock>&, const APESEARCH::string& );
    };

    SetOfUrls set;
    using FrontierCircBuf = APSEARCH::circular_buffer< APESEARCH::Func, APESEARCH::dynamicBuffer< APESEARCH::Func > >;
      APESEARCH::PThreadPool<FrontierCircBuf> pool; // The main threads that serve tasks
    std::atomic<bool> liveliness;
    
    static constexpr std::size_t frontQueueSize = 1024;
    // "To keep crawling threds busy, 3 times as many backeended queues as crawler threads"
    static unsigned ratingOfTopLevelDomain( const char * );

public:
    FrontEndPrioritizer frontEnd;
    BackendPolitenessPolicy backEnd;
    UrlFrontier() = default;
    //UrlFrontier(  );
    UrlFrontier( const char *, const size_t numOfCrawlerThreads );
    APESEARCH::string getNextUrl( );
    // Will assume that bloom filter is already accounted for ( Node actually owns the bloomfilter here )
    bool insertNewUrl( APESEARCH::string&& url );

    void shutdown(); // signals threads to stop
};

#endif