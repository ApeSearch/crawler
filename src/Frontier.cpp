
#include "../include/crawler/Frontier.h"
#include <assert.h>
#include <random>
#include <utility>

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

// Returns a timepoint about 10 seconds into the future...
static std::chrono::time_point<std::chrono::system_clock> newTime(  )
   {
   auto startMs = std::chrono::time_point_cast<std::chrono::seconds>( std::chrono::system_clock::now() );

   long conversion = startMs.time_since_epoch().count();
   conversion += 10;

   std::chrono::seconds dur( conversion );
   return std::chrono::time_point<std::chrono::system_clock>( dur );
   } // end newTime()

inline std::size_t UrlFrontier::FrontEndPrioritizer::pickQueue( )
   {
   //srand( time( 0 ) );
   //int num = rand() % 100;
   return 0;
   } // end pickQueue()

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
      if ( pQueues[ urlOf.priority ].pQueue.size() == FrontEndPrioritizer::urlsPerPriority )
         set.enqueue( urlOf.url );
      else
         break;

      if ( ++attempts == attemptsBefSleep )
         {
         attempts = 0;
         sleep( 30 ); // sleep to give chance to other queues
         } // end if
      // Try again
      } // end while 
   
   return urlOf;
   } // end helperReadInUrl()

// Thread that continously runs until Frontier is shut down...
void UrlFrontier::FrontEndPrioritizer::readInUrl( SetOfUrls& set, std::atomic<bool>& liveliness )
   {
   UrlObj url;
   while( liveliness.load() )
      {
      empty.down();
      url = helperReadInUrl( set, liveliness ); // Runs until it gets for a queue that's not empty
      pQueues[ url.priority ].pQueue.push( std::move( url.url ) );
      full.up();
      } // end while
   } // end readInUrl()

// Called whenever a thread is 
APESEARCH::string UrlFrontier::FrontEndPrioritizer::getUrl( )
   {
   // Will always be guaranteed a url
   full.down(); 
   size_t ind = pickQueue();
   APESEARCH::unique_lock<APESEARCH::mutex> lk( pQueues[ ind ].queueLk );

   APESEARCH::queue<APESEARCH::string, APESEARCH::circular_buffer< APESEARCH::string, 
                APESEARCH::DEFAULT::defaultBuffer< APESEARCH::string, urlsPerPriority> > > *queue = nullptr;

   if ( pQueues[ ind ].pQueue.empty() )
      {
      lk.unlock();
      std::cout << "Empty fill in with an asynchronous thread\n";
      // Start from the highest priority all the way to zero
      int n;
      for ( n = int ( pQueues.size() - 1 ); n >= 0; --n ) 
         {
         // Skip the own queue
         if ( n != ( int )ind )
            {
            lk = APESEARCH::unique_lock<APESEARCH::mutex> ( pQueues[ ( unsigned ) n  ].queueLk );
            if ( !pQueues[ ( unsigned ) n ].pQueue.empty() )
               queue = &pQueues[ ( unsigned ) n ].pQueue;

            } // end if
         } // end for
      assert( n >= 0 );
      } // end if
   else
      queue = &pQueues[ ind ].pQueue;
   
   assert( queue && queue->empty() );
   APESEARCH::string url( std::move( queue->front() ) );
   queue->pop();
   // There is one less url in the frontend queue now( ask to fill er up )
   empty.up();
   return url;
   } // end getUrl()

UrlFrontier::BackendPolitenessPolicy::BackendPolitenessPolicy( const size_t numOfQueues )
   :  domainQueues( numOfQueues ), semaHeap( 0 ) { }

// No need for lock since it's impossible for another thread to pop from this queue
// Inserts its another time domain if new, otherwise, 
// If domain is empty, accept any url,
// otherwise, insert into any queue up to the one in which 
// map lock -> queue lock
void UrlFrontier::BackendPolitenessPolicy::fillUpEmptyBackQueue( FrontEndPrioritizer& frontEnd, SetOfUrls& set, 
   const size_t index, std::string&& domain )
   {
   APESEARCH::unique_lock< APESEARCH::mutex > qLk( domainQueues[ index ].queueWLk.queueLk );
   std::unordered_map< std::string, size_t >::iterator itr;

   auto& queue = domainQueues[ index ].queueWLk.pQueue;
   for ( ;queue.empty(); qLk.lock() )
      {
      qLk.unlock();
      APESEARCH::string url( frontEnd.getUrl( ) ); 
      ParsedUrl parsedUrl( url.cstr() );
      if ( *parsedUrl.Host )
         {
         unsigned indToInsert = 0;
         APESEARCH::unique_lock< APESEARCH::mutex > uniqMLk( mapLk );
         APESEARCH::unique_lock< APESEARCH::mutex > backEndLk;

         std::string extractedDomain( parsedUrl.Host, parsedUrl.Port );
         itr = domainsMap.find( extractedDomain );
         if ( itr != domainsMap.end() )
            {
            backEndLk = APESEARCH::unique_lock< APESEARCH::mutex >( domainQueues[ itr->second ].queueWLk.queueLk );
            uniqMLk.unlock();
            indToInsert = itr->second;
            } // end if
         // Need to replace domain
         else
            {
            indToInsert = index;
            // A previously inserted domain
            if ( !domain.empty() )
               {
               itr = domainsMap.find( std::string( domain.begin(), domain.end() ) );
               assert( itr != domainsMap.end() );
               domainsMap.erase( itr );
               } // end if
            // Insert a new entry to map
            domainMap.emplace( std::piecewise_construct, 
               std::tuple<char *, char*>(  parsedUrl.Host, parsedUrl.Port ), std::tuple<unsigned>( index ) );
            backEndLk = APESEARCH::unique_lock< APESEARCH::mutex >( domainQueues[ index ].queueWLk.queueLk );
            uniqMLk.unlock(); // Must happen after obtaining the queue lock

            APESEARCH::unique_lock< APESEARCH::mutex > uniqPQLk( pqLk );
            backendHeap.emplace( newTime(), index );
            semaHeap.up();
            uniqPQLk.unlock();
            domainQueues[ index ].domain = std::move( extractedDomain ); // okay to steal now
            domainQueues[ index ].timeStampInDomain = true;
            } // end else

         // If it's full, need to return back to frontier
         if ( domainQueues[ indToInsert ].queueWLk.pQueue.size() == BackendPolitenessPolicy::endQueueSize )
            set.enqueue( url );
         else
            domainQueues[ indToInsert ].queueWLk.pQueue.emplace( std::move( url ) );
         } // end if
      } // end for
   // Notify any waiting threads...
   domainQueues[ index ].queueCV.notify_one();
   } // end fillUpEmptyBackQueue()

// queue lock -> priority queue lock
// map lock -> queue lock 
void UrlFrontier::BackendPolitenessPolicy::insertTiming( const std::chrono::time_point<std::chrono::system_clock>& time, const std::string& domain )
   {
   APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
   std::unordered_map<std::string, size_t>::iterator itr = domainsMap.find( domain );

   // Basically forget about it...
   while ( itr != domainsMap.end() )
      {
      unsigned ind = itr->second;
      assert( ind < domainQueues.size() );
      auto& pQueueOf = domainQueues[ ind ].queueWLk;
      APESEARCH::unique_lock<APESEARCH::mutex> uniqQLk( pQueueOf.queueLk );
      uniqMapLk.unlock(); // Okay to unlock now

      // Wait until it is appropriate to continue
      auto cond = [this, &domain, ind]() -> bool {
         return !domainQueues[ ind ].queueWLk.pQueue.empty() || domain != domainQueues[ ind ].domain; };
      domainQueues[ ind ].queueCV.wait( uniqQLk, cond );

      if ( !pQueueOf.pQueue.empty( ) && domain == domainQueues[ ind ].domain )
         {
         APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );
         backendHeap.emplace( time, itr->second );
         semaHeap.up(); // Okay for waiting threads to proceed
         } // end if
      } // end while
   } // end insertTiming()

APESEARCH::pair< APESEARCH::string, size_t > UrlFrontier::BackendPolitenessPolicy::getMostOkayUrl( SetOfUrls& set )
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
   assert( index < domainQueues.size() );
   APESEARCH::unique_lock<APESEARCH::mutex> backEndLk( domainQueues[ index ].queueWLk.queueLk );

   // Modify all of back queue's state
   APESEARCH::string url( std::move( domainQueues[ index ].queueWLk.pQueue.front() )  );
   domainQueues[ index ].queueWLk.pQueue.pop();
   domainQueues[ index ].timeStampInDomain = false;

   bool isEmpty =  domainQueues[ index ].queueWLk.pQueue.empty();

   return APESEARCH::pair< APESEARCH::string, size_t >( url, isEmpty ? index : domainQueues.size()  );
   } // getMostOkayUrl()

UrlFrontier::UrlFrontier( const size_t numOfCrawlerThreads ) : set( ),  backEnd( numOfCrawlerThreads * 3 ), liveliness( true )
   {
   // Need to start up threads...
   startUp();
   }


UrlFrontier::UrlFrontier( const char *directory, const size_t numOfCrawlerThreads ) : set( directory ), backEnd( numOfCrawlerThreads * 3 ), liveliness( true )
   {
   // Need to start up threads...
   startUp();
   }

// Needs to startup 
// 1) readInUrl
// 2) fillUpEmptyBackQueue equal to the number of backEndQueues
void UrlFrontier::startUp()
   {
   // Submit readInUrl...

   auto urlReaderIn = [this]( )
      { this->frontEnd.readInUrl( set, liveliness ); };
   pool.submitNoFuture( urlReaderIn );

   // Have each backend Queue attempt to fill up queue
   auto fillUpQueues = [this] ( const size_t index )
      { this->backEnd.fillUpEmptyBackQueue( frontEnd, set, index, std::string() ) };
   for ( unsigned n = 0; n < backEnd.domainQueues.size(); ++n )  
      pool.submitNoFuture( fillUpQueues, n );

   } // end startUp( )

APESEARCH::string UrlFrontier::getNextUrl( )
   {
   APESEARCH::pair< APESEARCH::string, size_t > retObj( backEnd.getMostOkayUrl( frontEnd ,set, pool ) );
   assert( !retObj.first().empty() );

   // Check if queue is empty...
   unsigned ind = retObj.second();
   if ( ind != backEnd.domainQueues.size() )
      {
      APESEARCH::unique_lock< APESEARCH::mutex > uniqQLk( backEnd.domainQueues[ ind ].queueWLk.queueLk );
      assert( !backEnd.domainQueues[ ind ].timeStampInDomain );

      auto func = [ this ]( const size_t index, std::string&&domain )
         { this->backEnd.fillUpEmptyBackQueue( frontEnd, set, index, 
            std::forward< std::string>( domain ) ); };
      pool.submitNoFuture( func, ind, std::move( backEnd.domainQueues[ ind ].domain ) );
      } // end if

   return retObj.first();
   } // end getNextUrl()

bool UrlFrontier::insertNewUrl( APESEARCH::string&& url )
   {
   set.enqueue( url );
   return true;
   } // end insertNewUrl()
