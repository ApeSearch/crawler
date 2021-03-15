
#pragma once

#ifndef FRONTIER_H_APESEARCH
#define FRONTIER_H_APESEARCH

#include <cstddef> // for std::size_t
#include <queue>
using std::priority_queue;
#include <unordered_map>
using std::unordered_map;
#include <string>
#include <vector>
#include "../libraries/AS/include/AS/queue.h"
#include "../libraries/AS/include/AS/circular_buffer.h"
#include "../libraries/AS/include/AS/unique_ptr.h"

#ifdef testing
    #include <string>
    using std::string;
#else
    #include "../libraries/AS/include/AS/queue.h"
    #include "../libraries/AS/include/AS/string.h"
    //using APESEARCH::priority_queue;
#endif

#include<chrono>

struct domainTiming
{
    std::chrono::time_point<std::chrono::system_clock> timeWCanCrawl; //Indicates moment when crawlable. Value is computed based on current time
    string domain;
    //TODO implement comparator

};

struct UrlObj
{

    struct PriorityFields
    {
    size_t num;
    bool seenInBloomFilter;
    };

    string url;
    PriorityFields priority; // Indicate which bucket to place priority
};

class UrlFrontier
{
    // Representation of the front-end queues
    class FrontEndPrioritizer 
    {
        static const constexpr std::size_t urlsPerPriority = 1000;
        std::vector<APESEARCH::queue<UrlObj, 
                APESEARCH::circular_buffer< UrlObj, 
                APESEARCH::DEFAULT::defaultBuffer< UrlObj, urlsPerPriority>
                                        pQueues;
        char * urlsToCrawl_front; // memory-mapped queue of urls to be crawled
        class Impl;
        APESEARCH::unique_ptr<Impl> impl;
        std::size_t pickQueue(); // uer-defined prirority for picking which queue to pop from
    public:
        UrlObj getUrl();
        void putUrl();
    };

    class BackendPolitenessPolicy
    {
        static constexpr std::size_t endQueueSize = 100;
        static constexpr std::size_t amtEndQueues = 3000; // This assumes 1000 crawlers
        std::priority_queue<domainTiming> backendHeap;
        std::unordered_map<string, size_t> backendDomains;
        std::vector<APESEARCH::queue<UrlObj, 
                APESEARCH::circular_buffer< UrlObj, 
                APESEARCH::DEFAULT::defaultBuffer< UrlObj, endQueueSize>
                                        domainQueues;
        UrlObj obtainRandUrl();
    public:
        UrlObj getMostOkayUrl();
    };
    FrontEndPrioritizer frontEnd;
    BackendPolitenessPolicy backEnd;

    
    static constexpr std::size_t frontQueueSize = 1000;
    // "To keep craling threds busy, 3 times as many backeended queus as crawler threads"
    class Impl;
    APESEARCH::unique_ptr<Impl> impl;
public:
    UrlFrontier() = default;
    void run();
};















#endif