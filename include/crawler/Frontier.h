
#pragma once

#ifndef FRONTIER_H_APESEARCH
#define FRONTIER_H_APESEARCH

#include <cstddef> // for std::size_t
#include <unordered_map>
#include <string>

#include "../libraries/AS/include/AS/vector.h"
#include "../libraries/AS/include/AS/queue.h"
#include "../libraries/AS/include/AS/circular_buffer.h"
#include "../libraries/AS/include/AS/unique_ptr.h"
#include "../libraries/AS/include/AS/string.h"
#include "ParsedUrl.h"

#include<chrono>

struct domainTiming
{
    std::chrono::time_point<std::chrono::system_clock> timeWCanCrawl; //Indicates moment when crawlable. Value is computed based on current time
    APESEARCH::string domain;
    //TODO implement comparator
};

struct UrlObj
{
    struct PriorityFields
    {
    size_t num;
    bool seenInBloomFilter;
    };

    APESEARCH::string url;
    ParsedUrl parsedUrl;
    PriorityFields priority; // Indicate which bucket to place priority
};

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
        APESEARCH::priority_queue<domainTiming> backendHeap;
        std::unordered_map<APESEARCH::string, size_t> backendDomains;
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

    
    static constexpr std::size_t frontQueueSize = 1024;
    // "To keep craling threds busy, 3 times as many backeended queus as crawler threads"
    class Impl;
    APESEARCH::unique_ptr<Impl> impl;
public:
    UrlFrontier() = default;
    void run();
};















#endif