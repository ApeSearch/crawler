
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
#define SECSTOWAIT 1

// Returns a timepoint about 10 seconds into the future...
std::chrono::time_point<std::chrono::system_clock> newTime( )
   {
   auto startMs = std::chrono::time_point_cast<std::chrono::seconds>( std::chrono::system_clock::now() );

   long conversion = startMs.time_since_epoch().count();
   conversion += SECSTOWAIT;

   std::chrono::seconds dur( conversion );
   std::chrono::time_point<std::chrono::system_clock> finalDt( dur );
   return finalDt;
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

      if ( !url.url.empty( ) )
         {
         pQueues[ url.priority ].pQueue.push( std::move( url.url ) );
         full.up();
         }
      } // end while
   } // end readInUrl()

// Called whenever a thread is 
APESEARCH::string UrlFrontier::FrontEndPrioritizer::getUrl( )
   {
   // Will always be guaranteed a url ( as long as liveliness == true )
   full.down(); 
   if ( !liveliness.load() )
      return "";

   APESEARCH::queue<APESEARCH::string, APESEARCH::circular_buffer< APESEARCH::string, 
                APESEARCH::DEFAULT::defaultBuffer< APESEARCH::string, urlsPerPriority> > > *queue = nullptr;
   size_t ind = pickQueue();
   APESEARCH::unique_lock<APESEARCH::mutex> lk( pQueues[ ind ].queueLk );
   if ( pQueues[ ind ].pQueue.empty() )
      {
      lk.unlock();
#ifdef DEBUG
      std::cerr << "Empty fill in with an asynchronous thread\n";
#endif
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

   assert( queue && !queue->empty() );
   APESEARCH::string url = std::move( queue->front() );
   queue->pop();
   empty.up();
   // There is one less url in the frontend queue now( ask to fill er up )
   return url;
   } // end getUrl()

UrlFrontier::FrontEndPrioritizer::~FrontEndPrioritizer()
   {
   
   for ( QueueWLock< urlsPerPriority >& queue : pQueues )
      {
      APESEARCH::unique_lock<APESEARCH::mutex> qlk( queue.queueLk );
      while( !queue.pQueue.empty( ) )
         {
         setRef.enqueue( queue.pQueue.front( ) );
         queue.pQueue.pop( );
         } // end while
      } // end for
   } // end ~FrontEndPrioritizer( )

UrlFrontier::BackendPolitenessPolicy::BackendPolitenessPolicy( const size_t numOfQueues, SetOfUrls& _set, std::atomic<bool>& boolean )
   :  domainQueues( numOfQueues ), semaHeap( 0 ), setRef( _set ), liveliness( boolean ) { }

UrlFrontier::BackendPolitenessPolicy::~BackendPolitenessPolicy( )
   {
   for( BackEndQueue& queue : domainQueues )
      {
      QueueWLock< endQueueSize >& lkQueue = queue.queueWLk;
      APESEARCH::unique_lock<APESEARCH::mutex> lk( lkQueue.queueLk );
      while( !lkQueue.pQueue.empty( ) )
         {
         setRef.enqueue( lkQueue.pQueue.front( ) );
         lkQueue.pQueue.pop( );
         } // end while
      } // end for
   }

// No need for lock since it's impossible for another thread to pop from this queue
// Inserts its another time domain if new, otherwise, 
// If domain is empty, accept any url,
// otherwise, insert into any queue up to the one in which 
// map lock -> queue lock
void UrlFrontier::BackendPolitenessPolicy::fillUpEmptyBackQueue( FrontEndPrioritizer& frontEnd, SetOfUrls& set, 
   const size_t index, const std::string& domain )
   {
   APESEARCH::unique_lock< APESEARCH::mutex > qLk( domainQueues[ index ].queueWLk.queueLk );
   std::unordered_map< std::string, size_t >::iterator itr;

   auto& queue = domainQueues[ index ].queueWLk.pQueue;
   while ( liveliness.load() && queue.empty() )
      {
      qLk.unlock();
      APESEARCH::string url( frontEnd.getUrl( ) ); 
      ParsedUrl parsedUrl( url.cstr() );
      if ( !url.empty( ) && *parsedUrl.Host )
         {
         unsigned indToInsert = 0;
         std::string extractedDomain( parsedUrl.Host, parsedUrl.Port );

         APESEARCH::unique_lock< APESEARCH::mutex > uniqMLk( mapLk );
         APESEARCH::unique_lock< APESEARCH::mutex > backEndLk;
         itr = domainsMap.find( extractedDomain );
         if ( itr != domainsMap.end() )
            {
            if ( itr->second == index )
               qLk.lock( );
            else
               backEndLk = APESEARCH::unique_lock< APESEARCH::mutex >( domainQueues[ itr->second ].queueWLk.queueLk );

            uniqMLk.unlock();
            indToInsert = itr->second;
            } // end if
         // Need to replace domain
         else
            {
            indToInsert = index;
            // Delete previously domain from map
            if ( !domain.empty() )
               {
               itr = domainsMap.find( std::string( domain.begin(), domain.end() ) );
               assert( itr != domainsMap.end() && itr->second == index );
               domainsMap.erase( itr );
               } // end if
            // Insert a new entry to map
            domainsMap.emplace( std::piecewise_construct, 
               std::tuple<char *, char*>(  parsedUrl.Host, parsedUrl.Port ), std::tuple<unsigned>( index ) );

            qLk.lock( );
            uniqMLk.unlock(); // Must happen after obtaining the queue lock

            // Reinsert a fresh new timestamp
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
         
         if ( itr != domainsMap.end() && itr->second != index )
            qLk.lock( );
         } // end if
      else
         qLk.lock( );
      } // end for
   // Notify any waiting threads...
   domainQueues[ index ].queueCV.notify_one();
   } // end fillUpEmptyBackQueue()

// queue lock -> priority queue lock
// map lock -> queue lock 
bool UrlFrontier::BackendPolitenessPolicy::insertTiming( const std::chrono::time_point<std::chrono::system_clock>& time, const std::string& domain )
   {
   APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
   // Basically forget about it... ( if itr == domainsMap.end( ) )
   for ( std::unordered_map<std::string, size_t>::iterator itr; 
      liveliness.load() && ( itr = domainsMap.find( domain ) ) != domainsMap.end(); uniqMapLk.lock( ) )
      {
      unsigned ind = itr->second;
      assert( ind < domainQueues.size() );
      auto& pQueueOf = domainQueues[ ind ].queueWLk;
      APESEARCH::unique_lock<APESEARCH::mutex> uniqQLk( pQueueOf.queueLk );
      uniqMapLk.unlock(); // Okay to unlock now

      // Wait until it is appropriate to continue

      assert( domain == domainQueues[ ind ].domain );
      auto cond = [this, &domain, ind]() -> bool {
         return ( !liveliness.load( ) || !domainQueues[ ind ].queueWLk.pQueue.empty() || domain != domainQueues[ ind ].domain ); };
      domainQueues[ ind ].queueCV.wait( uniqQLk, cond );

      // If this is the case, then !domainQueues[ ind ].queueWLk.pQueue.empty()
      if ( domain == domainQueues[ ind ].domain )
         {
         if ( !domainQueues[ ind ].timeStampInDomain )
            {
            APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );
            backendHeap.emplace( time, itr->second );
            domainQueues[ ind ].timeStampInDomain = true;
            semaHeap.up(); // Okay for waiting threads to proceed    
            return true;
            }
         // Indicates failure
         return false;
         } // end if
      } // end while
   return false;
   } // end insertTiming()

APESEARCH::pair< APESEARCH::string, size_t > UrlFrontier::BackendPolitenessPolicy::getMostOkayUrl( SetOfUrls& set )
   {
   unsigned index = 0;
{
   semaHeap.down(); // Okay to go now that there's an open backend queue
   APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );

   // wait until time has reached the past before popping...
   while ( !cvHeap.wait_until( uniqPQLk, backendHeap.top().timeWCanCrawl, 
      [this](){ 
         auto timeNow = std::chrono::time_point_cast<std::chrono::seconds>( std::chrono::system_clock::now() );
         return backendHeap.top().timeWCanCrawl < timeNow; } ) );
   
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

UrlFrontier::UrlFrontier( const size_t numOfCrawlerThreads ) : UrlFrontier( nullptr, numOfCrawlerThreads ) { }


UrlFrontier::UrlFrontier( const char *directory, const size_t numOfCrawlerThreads ) : set( directory ),  pool( FrontierCircBuf( numOfCrawlerThreads * 3 ), numOfCrawlerThreads  * 3, computeTwosPowCeiling( numOfCrawlerThreads * 6 ) )
   ,liveliness( true ), frontEnd( set, liveliness ), backEnd( numOfCrawlerThreads * 3, set, liveliness )
   {
   // Need to start up threads...
   startUp();
   }

UrlFrontier::~UrlFrontier( )
   {
   shutdown();
   } // end UrlFrontier()

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
      { this->backEnd.fillUpEmptyBackQueue( frontEnd, set, index, std::string() ); };
   for ( unsigned n = 0; n < backEnd.domainQueues.size(); ++n )  
      pool.submitNoFuture( fillUpQueues, n );

   } // end startUp( )

void UrlFrontier::shutdown( )
   {
   liveliness.store( false );
   set.shutdown( );
   ssize_t count = frontEnd.empty.getCount( );
   if ( count < 0 )
      frontEnd.empty.up( -count );

   count = frontEnd.full.getCount( );
   if ( count < 0 )
      frontEnd.full.up( -count );
   std::cout << "Shutting down url frontier\n";
   pool.shutdown( );
   } // end shutdown( )

APESEARCH::string UrlFrontier::getNextUrl( )
   {
   APESEARCH::pair< APESEARCH::string, size_t > retObj( backEnd.getMostOkayUrl( set ) );
   assert( !retObj.first().empty() );

   // Check if queue is empty...
   unsigned ind = retObj.second();
   if ( ind != backEnd.domainQueues.size() )
      {
      APESEARCH::unique_lock< APESEARCH::mutex > uniqQLk( backEnd.domainQueues[ ind ].queueWLk.queueLk );
      assert( !backEnd.domainQueues[ ind ].timeStampInDomain );

      auto func = [ this, domain{ std::string( backEnd.domainQueues[ ind ].domain ) }]( const size_t index )
         { this->backEnd.fillUpEmptyBackQueue( frontEnd, set, index, domain ); };
      pool.submitNoFuture( func, ind );
      } // end if

   return retObj.first();
   } // end getNextUrl()

bool UrlFrontier::insertNewUrl( APESEARCH::string&& url )
   {
   set.enqueue( url );
   return true;
   } // end insertNewUrl()
