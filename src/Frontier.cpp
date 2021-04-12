
#include "../include/crawler/Frontier.h"
#include <assert.h>
#include <random>

// .com
// .co
// .org
// .us
// .net
// .blog
// .io
// .gov
// .edu
// Any other domain

inline std::size_t UrlFrontier::FrontEndPrioritizer::pickQueue( )
   {
   //srand( time( 0 ) );
   //int num = rand() % 100;
   return 0;
   }

// Keeps trying until it gets a url that fits in one of the queues
UrlObj UrlFrontier::FrontEndPrioritizer::helperReadInUrl( SetOfUrls& set, std::atomic<bool>& liveliness )
   {
   static const unsigned attemptsBefSleep = 50;
   unsigned attempts = 0;
   UrlObj urlOf;
   while( liveliness.load() )
      {
      urlOf = set.blockingDequeue( ); // Potentially blocking
      APESEARCH::unique_lock<APESEARCH::mutex> lk( pQueues[ urlOf.priority ].queueLk );
      if ( pQueues[ urlOf.priority ].pQueue.size() == QueueWLock::urlsPerPriority )
         set.enqueue( urlOf.url );

      if ( ++attempts == attemptsBefSleep )
         {
         attempts = 0;
         sleep( 30 ); // sleep to prevent busy waiting
         } // end if
      // Try again
      } // end while 
   } // end helperReadInUrl()

// Thread that continously runs until Frontier is shut down...
void UrlFrontier::FrontEndPrioritizer::readInUrl( SetOfUrls& set, std::atomic<bool>& liveliness )
   {
   UrlObj url;
   while( liveliness.load() )
      {
      empty.down();
      url = helperReadInUrl(); // Runs until it gets for a queue that's not empty
      pQueues[ url.priority ].push( std::move( url.url ) );
      full.up();
      } // end while
   } // end readInUrl()

APESEARCH::string UrlFrontier::FrontEndPrioritizer::getUrl( )
   {
   // Will always be guaranteed a url
   full.down(); 
   unsigned ind = pickQueue();
   APESEARCH::unique_lock<APESEARCH::mutex> lk( pQueues[ ind ].queueLk );
   auto *queue = nullptr;
   if ( pQueues[ ind ].pQueue.empty() )
      {
      lk.unlock();
      std::cout << "Empty fill in with an asynchronous thread\n";
      // Start from the highest priority all the way to zero
      for ( int n = pQueues.size() - 1; n >= 0; --n ) 
         {
         // Skip the own queue
         if ( n != ind )
            {
            lk = APESEARCH::unique_lock<APESEARCH::mutex> qlk( pQueues[ ( unsigned ) n  ].queueLk );
            if ( !pQueues[ ( unsigned ) n ].pQueue.empty() )
               {
               queue = &pQueues[ ( unsigned ) n ].pQueue;
               } // end if
            } // end if
         } // end for
      } // end if
   else
      queue = &pQueues[ ind ].pQueue;
   
   assert( queue && pQueue->empty() );
   APESEARCH::string url( std::move( queue->front() ) );
   queue->pop();
   // There is one less url in the frontend queue now( ask to fill er up )
   empty.up();
   return url;
   } // end getUrl()

// No need for lock since it's impossible for another thread to pop from this queue
// Inserts its another time domain if new, otherwise, 
// If domain is empty, accept any url,
// otherwise, insert into any queue up to the one in which 
// queue lock -> map lock
void UrlFrontier::BackendPolitenessPolicy::fillUpEmptyBackQueue( FrontEndPrioritizer& frontEnd, APESEARCH::string&& domain, const size_t index )
   {
   APESEARCH::unique_lock< APESEARCH::mutex > qLk( domainQueues[ index ].queueWLk.queueLk );
   APESEARCH::unordered_map< std::string, size_t >::iterator itr; //= domainsMap.find( domain );
{
   APESEARCH::unique_lock< APESEARCH::mutex > uniqMLk( mapLk );
   itr = domainsMap.find( std::string( domain.begin(), domain.end() ) );
}
   auto& queue = domainQueues[ index ].queueWLk.pQueue;
   while( queue.empty() )
      {
      qLk.unlock();
      }
   if ( itr != domainsMap.end() )
      {
      if ( itr->second == index )
         {
         
         }
      APESEARCH::string url( frontEnd.getUrl() );

      } // end if
   if ( domainQueues[ index ].queueWLk.pQueue.empty() )
      {
      // Now continously try to insert into queue...


      } // end if
   } // end fillUpEmptyBackQueue()

// queue lock -> priority queue lock
// queue lock -> map lock
void UrlFrontier::BackendPolitenessPolicy::insertTiming( const std::chrono::time_point<std::chrono::system_clock>& time, const APESEARCH::string& domain )
   {
   std::unordered_map<std::string, size_t>::iterator itr;
   std::string str( domain.cbegin(), domain.cend() );
{
   APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
   itr = domainsMap.find( str );
} // ~uniqMapLk
   if ( itr != domainsMap.end() )
      {
      unsigned ind = itr->second;
      auto& pQueueOf = domainQueues[ ind ].queueWLk;
      APESEARCH::unique_lock<APESEARCH::mutex> uniqQLk( pQueueOf.queueLk );
      if ( !isInsertedToheap && pQueueOf.pQueue.empty() )
         {
         
         // Want to return from a condition where 
         // 1) is either not in the map anymore, 
         auto cond = [this, &str, ind]() -> bool {
            APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
            std::unordered_map<std::string, size_t>::iterator itr = domainsMap.find( str );
            bool retVal = ( itr == domainsMap.end() || itr->second != ind );
            // If not true, needs to check if queue is empty or not
            if ( itr != domainsMap.end()  )
               {
               return;
               } // end if
         } // end if
      APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );
      backendHeap.emplace( time, itr->second );
      };

      return true;
      } // end if
   return false;
   } // end insertTiming()

UrlObj UrlFrontier::BackendPolitenessPolicy::obtainRandUrl()
   {
   
   return UrlObj();
   }

APESEARCH::string UrlFrontier::BackendPolitenessPolicy::getMostOkayUrl( SetOfUrls& set,  APESEARCH::PThreadPool<FrontierCircBuf>& pool )
   {
   unsigned index = 0;
{
   semaHeap.down(); // Okay to go now that there's an open backend queue
   APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );

   // wait until time has reached the past before popping...
   while ( cvHeap.wait_until( uniqPQLk, backendHeap.top().timeWCanCrawl, 
      [this](){ return backendHeap.top().timeWCanCrawl < 
         std::chrono::time_point_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() ) } ) );
   
   // Pop Queue...
   index = backendHeap.top().index;
   backendHeap.pop();
} // ~uniqPQLk()
   assert( ind < BackendPolitenessPolicy::endQueueSize );
   APESEARCH::unique_lock<APESEARCH::mutex> backEndLk( domainQueues[ ind ].queueWLk.queueLk );
   APESEARCH::string url( std::move( domainQueues[ ind ].queueWLk.pQueue.front() )  );
   domainQueues[ ind ].queueWLk.pQueue.pop();

   if ( domainQueues[ ind ].queueWLk.pQueue.empty() )
      {
      APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
      std::unordered_map<std::string, size_t>::iterator itr = domainsMap.find( str );
      pool.submitNoFuture(  );
      }

   return UrlObj();
   }

unsigned UrlFrontier::ratingOfTopLevelDomain( const char * )
   {
    return 1;
   }

UrlFrontier::UrlFrontier( const char *directory, const size_t numOfCrawlerThreads ) : set( directory ), backEnd(  )
   {
   // Need to start up threads...
   }

APESEARCH::string UrlFrontier::getNextUrl( )
   {
   UrlObj obj( backEnd.getMostOkayUrl( set ) );
   return obj.url;
   }
bool UrlFrontier::insertNewUrl( APESEARCH::string&& url )
   {
   return true;
   }
