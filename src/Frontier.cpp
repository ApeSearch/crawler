
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
//static APESEARCH::mutex coutLk;
#define SECSTOWAIT 7

std::atomic<size_t> queuesChosen[ SetOfUrls::maxPriority ];

// Returns a timepoint about 7 seconds into the future...
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
   static const APESEARCH::vector< unsigned > discreteDist = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1 };
   unsigned num = rand() & discreteDist.size( ) - 1;
   return discreteDist[ num ];
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
   
{
      APESEARCH::unique_lock<APESEARCH::mutex> lk( pQueues[ urlOf.priority ].queueLk );
      if ( pQueues[ urlOf.priority ].pQueue.size() == FrontEndPrioritizer::urlsPerPriority )
         set.enqueue( urlOf.url );
      else
         break;
}
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
         APESEARCH::unique_lock<APESEARCH::mutex> lk( pQueues[ url.priority ].queueLk );
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

      // Start from the highest priority all the way to zero
      ind = 0;
      do
      {
         lk = APESEARCH::unique_lock<APESEARCH::mutex> ( pQueues[ ind  ].queueLk );
         if ( !pQueues[ ind ].pQueue.empty() )
            {
            queue = &pQueues[ ind ].pQueue;
            break;
            } // end if
         ind = ( ind + 1 ) % pQueues.size( );
      } while ( true );
      } // end if
   else
      queue = &pQueues[ ind ].pQueue;
   queuesChosen[ ind ].fetch_add( 1 );

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
   while ( liveliness.load() && domainQueues[ index ].domain == domain && queue.empty() )
      {
      qLk.unlock();
      APESEARCH::string url( frontEnd.getUrl( ) ); 
      ParsedUrl parsedUrl( url.cstr() );
      // Used to check if https ( now done in helperDeq )
      if ( *parsedUrl.Host )
         {
         unsigned indToInsert = 0;
         std::string extractedDomain( parsedUrl.Host, parsedUrl.Port );

         APESEARCH::unique_lock< APESEARCH::mutex > uniqMLk( mapLk );
         APESEARCH::unique_lock< APESEARCH::mutex > backEndLk;
         itr = domainsMap.find( extractedDomain );
         if ( itr != domainsMap.end() )
            {
            indToInsert = itr->second;
            if ( indToInsert == index )
               qLk.lock( );
            else
               backEndLk = APESEARCH::unique_lock< APESEARCH::mutex >( domainQueues[ indToInsert ].queueWLk.queueLk );

            uniqMLk.unlock();
            } // end if
         // Need to replace domain
         else
            {
            indToInsert = index;
            qLk.lock( );
            // Delete previously domain from map
            if ( !domain.empty() )
               {
               // In the case that no longer has queue associated or the state of queue has changed before ( no longer empty )
               // Another thread is now responsible...
               // Also needs to account for timeStampInDomain is true ( say returned back to domain ( from a previous one) )
               // Still needs to check
               if ( domainQueues[ index ].domain != domain || !queue.empty() || domainQueues[ index ].timeStampInDomain )
                  {
                  uniqMLk.unlock(); // Must happen after obtaining the queue lock
                  qLk.unlock( );
                  set.enqueue( url ); // Need to return back to frontier
                  return; // Pass responsiblity to another thread
                  } // end if
               itr = domainsMap.find( domain );
               assert( itr != domainsMap.end() );
               assert( indToInsert == index );

               domainsMap.erase( itr );
               } // end if
            // Insert a new entry to map
            domainsMap.emplace( std::piecewise_construct, 
               std::tuple<char *, char*>(  parsedUrl.Host, parsedUrl.Port ), std::tuple<unsigned>( index ) );
            itr = domainsMap.end( );
            assert( domainsMap.find( extractedDomain ) != domainsMap.end( ) );
            uniqMLk.unlock(); // Must happen after obtaining the queue lock

            // Reinsert a fresh new timestamp
            APESEARCH::unique_lock< APESEARCH::mutex > uniqPQLk( pqLk );
            backendHeap.emplace( newTime(), index );
            semaHeap.up();
            uniqPQLk.unlock();
            domainQueues[ index ].timeStampInDomain = true;
            domainQueues[ index ].domain = std::move( extractedDomain ); // okay to steal now
            } // end else

         // If it's full, need to return back to frontier
         if ( domainQueues[ indToInsert ].queueWLk.pQueue.size() == BackendPolitenessPolicy::endQueueSize )
            set.enqueue( url );
         else
            {
            domainQueues[ indToInsert ].queueWLk.pQueue.emplace( std::move( url ) );
            // Notify any waiting threads...
            domainQueues[ indToInsert ].queueCV.notify_one( );
            } // end else
         
         // Previously just checked if itr != end() && itr->second != index
         // which would be invalidated if itr was changed when itr == end
         if ( !qLk )
            qLk.lock( );
         } // end if
         else
            qLk.lock();
      } // end for
   } // end fillUpEmptyBackQueue()

// queue lock -> priority queue lock
// map lock -> queue lock 
bool UrlFrontier::BackendPolitenessPolicy::insertTiming( const std::chrono::time_point<std::chrono::system_clock>& time, const std::string& domain )
   {
   APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
   // Basically forget about it... ( if itr == domainsMap.end( ) )

   for ( std::unordered_map<std::string, size_t>::iterator itr; 
      liveliness.load() && ( ( itr = domainsMap.find( domain ) ) != domainsMap.end() ); uniqMapLk.lock( ) )
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
         assert( !domainQueues[ ind ].queueWLk.pQueue.empty() );
         if ( !domainQueues[ ind ].timeStampInDomain ) // Not in domainsHeap
            {
            APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );
            backendHeap.emplace( time, ind );
            domainQueues[ ind ].timeStampInDomain = true;
            semaHeap.up(); // Okay for waiting threads to proceed    
            return true;
            }
         // Indicates failure
         return false;
         } // end if
      } // end for
   return false;
   } // end insertTiming()

void UrlFrontier::initiateInsertToDomain( std::chrono::time_point<std::chrono::system_clock>&& time, std::string&& domain )
   {
   auto func = [this, time{ std::move( time ) }, domain{ std::move( domain ) } ]( )
      {
      this->backEnd.insertTiming( time, domain );
      };
   pool.submitNoFuture( func );
   }

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

   assert( domainQueues[ index ].timeStampInDomain );

   // Modify all of back queue's state
   APESEARCH::string url( std::move( domainQueues[ index ].queueWLk.pQueue.front() )  );
   domainQueues[ index ].queueWLk.pQueue.pop();
   domainQueues[ index ].timeStampInDomain = false;

   bool isEmpty =  domainQueues[ index ].queueWLk.pQueue.empty();

   // domainqueues.size( ) is a way to signify that a backend queue is empty and needs to be filled up
   // again.
   return APESEARCH::pair< APESEARCH::string, size_t >( url, isEmpty ? index : domainQueues.size()  );
   } // getMostOkayUrl()

UrlFrontier::UrlFrontier( const size_t numOfCrawlerThreads ) : UrlFrontier( nullptr, numOfCrawlerThreads ) { }


UrlFrontier::UrlFrontier( const char *directory, const size_t numOfCrawlerThreads ) : set( directory ),  pool( FrontierCircBuf( computeTwosPowCeiling( numOfCrawlerThreads * 6 ) ), ( numOfCrawlerThreads  * 3 ) * 2, computeTwosPowCeiling( numOfCrawlerThreads * 6 ) )
   ,liveliness( true ), frontEnd( set, liveliness ), backEnd( numOfCrawlerThreads * 3, set, liveliness )
   {
   // Need to start up threads...
   startUp();
   srand( time( 0 ) ); // Called only once
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
      //assert( !backEnd.domainQueues[ ind ].timeStampInDomain );

      auto func = [ this, domain{ std::string( backEnd.domainQueues[ ind ].domain ) }]( const size_t index )
         { this->backEnd.fillUpEmptyBackQueue( frontEnd, set, index, domain ); };
      pool.submitNoFuture( func, ind );
      } // end if

   return retObj.first();
   } // end getNextUrl()

bool UrlFrontier::insertNewUrl( APESEARCH::string&& url )
   {
   return insertNewUrl( url ); 
   }

bool UrlFrontier::insertNewUrl( const APESEARCH::string& url )
   {
   set.enqueue( url );
   return true;
   } // end insertNewUrl()
