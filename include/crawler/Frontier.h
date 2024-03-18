
#pragma once

#ifndef FRONTIER_H_APESEARCH
#define FRONTIER_H_APESEARCH

#include <cstddef> // for std::size_t
#include <unordered_map>
#include <string>
#include <atomic>
#include<chrono>
#include<ctime>
#include <sys/types.h>

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/queue.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/circular_buffer.h"
#include "../../libraries/AS/include/AS/unique_mmap.h"
#include "../../libraries/AS/include/AS/File.h"
#include "../../libraries/AS/include/AS/as_semaphore.h"
#include "../../libraries/AS/include/AS/pthread_pool.h"
#include "../../libraries/AS/include/AS/utility.h"
#include "ParsedUrl.h"
#include "SetOfUrls.h"
#include "DynamicBuffer.h"


#define NUMOFFRONTQUEUES 2

struct domainTiming
{
    //Indicates moment when crawlable. Value is computed based on current time
    std::chrono::time_point<std::chrono::system_clock> timeWCanCrawl; 
    unsigned index; // Stores index to queue
    //std::time_t timeWCanCrawl;

    domainTiming() = default;

    domainTiming( const std::chrono::time_point<std::chrono::system_clock>& _time, const unsigned _index )
        : timeWCanCrawl( _time ), index( _index ) {}
    
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
   };



class UrlFrontier
{
    //friend class APESEARCH::Mercator;
    // Representation of the front-end queues
    class FrontEndPrioritizer 
    {
    public:
        // Member Variables
        static const constexpr std::size_t urlsPerPriority = 2048;
        APESEARCH::vector< QueueWLock< urlsPerPriority > > pQueues;
        APESEARCH::semaphore empty;
        APESEARCH::semaphore full;
        SetOfUrls& setRef;
        std::atomic<bool>& liveliness;

        // Member Functions
        inline std::size_t pickQueue(); // user-defined prirority for picking which queue to pop from
        UrlObj helperReadInUrl( SetOfUrls& set, std::atomic<bool>& liveliness );
        // Thread that is continously trying to keep QueueWLock full at all times
        void readInUrl( SetOfUrls& set, std::atomic<bool>& );
        APESEARCH::string getUrl();

        FrontEndPrioritizer( SetOfUrls& _set, std::atomic<bool>& boolean, size_t numOfQueues = SetOfUrls::maxPriority ) : 
            pQueues( numOfQueues ), empty( FrontEndPrioritizer::urlsPerPriority * numOfQueues ), full( 0 ), setRef( _set ), liveliness( boolean ) {}
        ~FrontEndPrioritizer( );
    };

    class BackendPolitenessPolicy
    {
    public:
        // Member Variables
        static constexpr std::size_t endQueueSize = 128;
        struct BackEndQueue
            {
            QueueWLock< endQueueSize > queueWLk;
            std::string domain; // Used for insert timing to check once it's been woken up that queue it's sleeping on still has the same domain
            APESEARCH::condition_variable queueCV;
            bool timeStampInDomain = false;
            };
    
        static constexpr std::size_t amtEndQueues = 3000; // This assumes 1000 crawlers
        APESEARCH::priority_queue<domainTiming, 
            APESEARCH::BinaryPQ<domainTiming, domainTiming::compareTime >, 
            domainTiming::compareTime> backendHeap;
        APESEARCH::vector< BackEndQueue > domainQueues;
        std::unordered_map<std::string, size_t> domainsMap;
        APESEARCH::mutex pqLk;
        APESEARCH::mutex mapLk;
        // for priority queue
        APESEARCH::semaphore semaHeap;
        APESEARCH::condition_variable cvHeap; 
        SetOfUrls& setRef;
        std::atomic<bool>& liveliness;

        // Member Functions
        BackendPolitenessPolicy( const size_t numOfQueues, SetOfUrls& _set, std::atomic<bool>& );
        void fillUpEmptyBackQueue( FrontEndPrioritizer& frontEnd, SetOfUrls& set, 
            const size_t index, const std::string& domain );
        ~BackendPolitenessPolicy( );
        APESEARCH::pair< APESEARCH::string, size_t > getMostOkayUrl( SetOfUrls& );
        bool insertTiming( const std::chrono::time_point<std::chrono::system_clock>&, const std::string& );
    };

    using FrontierCircBuf = APESEARCH::circular_buffer< APESEARCH::Func, APESEARCH::dynamicBuffer< APESEARCH::Func > >;
    SetOfUrls set;
    
    // "To keep crawling threds busy, 3 times as many backeended queues as crawler threads"
    //static unsigned ratingOfTopLevelDomain( const char * );

    void startUp(); // Starts up threads...
public:
    APESEARCH::PThreadPool<FrontierCircBuf> pool; // The main threads that serve tasks
    std::atomic<bool> liveliness;

    FrontEndPrioritizer frontEnd;
    BackendPolitenessPolicy backEnd;
    UrlFrontier( const size_t numOfCrawlerThreads );
    UrlFrontier( const char *, const size_t numOfCrawlerThreads );
    ~UrlFrontier( );
    APESEARCH::string getNextUrl( );
    // Will assume that bloom filter is already accounted for ( Node actually owns the bloomfilter here )
    bool insertNewUrl( APESEARCH::string&& url );
    bool insertNewUrl( const APESEARCH::string& url );
    void initiateInsertToDomain( std::chrono::time_point<std::chrono::system_clock>&&, std::string&& );

    void shutdown(); // signals threads to stop
};

extern std::atomic<size_t> queuesChosen[ SetOfUrls::maxPriority ];
std::chrono::time_point<std::chrono::system_clock> newTime( );
#endif

