
#include "../include/crawler/Mercator.h"
#include "../Parser/HtmlParser.h"
//#include "../libraries/AS/include/AS/string.h"
#include <string> // needed to get idea out
#include <iostream>
#include <stdlib.h>     /* atoi */
#include <limits>

#define MULTIPLE 10

std::chrono::time_point<std::chrono::system_clock> getNewTime( const std::chrono::time_point<std::chrono::system_clock>& start, 
      const std::chrono::time_point<std::chrono::system_clock>& end )
   {
   auto startMs = std::chrono::time_point_cast<std::chrono::milliseconds>( start );
   auto endMs = std::chrono::time_point_cast<std::chrono::milliseconds>( end );

   auto startVal = startMs.time_since_epoch();
   auto endVal = endMs.time_since_epoch();

   long duration = endVal.count() - startVal.count();

   assert( duration >= 0 );

   duration *= MULTIPLE;

   long newStart = endVal.count() + duration;

   std::chrono::milliseconds dur( newStart );
   std::chrono::time_point<std::chrono::system_clock> dt( dur );

   return dt;
   } // end getNewTime

APESEARCH::Mercator::~Mercator()
   {
   cleanUp();
   }


void APESEARCH::Mercator::crawlWebsite( Request& requester, APESEARCH::string& buffer)
   {

   std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
   Result result = requester.getReqAndParse( buffer.cstr() );

{
   APESEARCH::unique_lock<APESEARCH::mutex> lk( lkForCrawled );
   size_t *num = ( size_t * ) pagesCrawled.get();
   ++(*num);
}   
   // At the end of this task, the buffer will be reinserted back into urlBuffers...
   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();

   std::chrono::time_point<std::chrono::system_clock> whenCanCrawlAgain( getNewTime( start, end ) );
   ParsedUrl parsedUrl( buffer.cstr() );

   frontier.pool.submitNoFuture( [this, whenCanCrawlAgain{ std::move( whenCanCrawlAgain ) }, domain{ std::string( parsedUrl.Host, parsedUrl.Port ) } ](  ) 
   { this->frontier.backEnd.insertTiming( whenCanCrawlAgain, domain ); } );   

   // This should be the case
   if( !*parsedUrl.Host )
      return;
   
   switch( result.status )
         {
         case getReqStatus::successful:
            {
            APESEARCH::vector< char > buf( requester.getResponseBuffer() );
            pool.submitNoFuture( [this, buffer{ std::move( buf ) }, url{ std::move( buffer ) } ]( )
            { this->parser( buffer, url ); } );
            break;
            }
         case getReqStatus::redirected:
            {
            // A quick fix to filter out http documents
            if( !strncmp( result.url.cstr(), "https", 5 ) && !bloomfilter.contains( result.url ) )
               {
               Link link;
               link.URL = std::move( result.url );
               auto func = [this, link{ std::move( link ) } ]( ) mutable { node.write( link ); };
               pool.submitNoFuture( func );
               } // end if
            }
            break;
         default:
            break;
         } // end switch
   } // end crawlWebsite()

// Only need one thread for this since it would otherwise 
// create contention...
void APESEARCH::Mercator::crawler( )
   {
    Request requester;

    while( true )
       {
        APESEARCH::string url = frontier.getNextUrl( ); // Writes directly to buffer
        if( url.empty( ) )
            continue;
        //std::cerr << "ACTUALLY GOT A URL\n";
        crawlWebsite( requester, url );
       } // end while
   } // end urlExtractor( )
   
void APESEARCH::Mercator::parser( const APESEARCH::vector< char >& buffer, const APESEARCH::string &url )
   {
   HtmlParser parser( buffer.begin( ), buffer.size(), url );

   // Handle results by writing to file...
   //TODO put this on own thread
   if ( parser.isValidHtml( ) )
      {
      writeToFile( parser );
   // Increment number of webpages written to disk
{
   APESEARCH::unique_lock<APESEARCH::mutex> lk( lkForWritten );
   size_t *num = ( size_t * ) pagesCrawled.get();
   ++num[ 1 ];
} 
   //std::cerr << "Crawled website successfully: " << url << '\n';
      } // end if

   } // end parser()

void APESEARCH::Mercator::writeToFile( HtmlParser& parser )
{
   //write to DB
   db.addParsedFile( parser );

   // In the case where the base is empty, it is (by default) such that
   // the base needs to be the url itself.
   if( parser.base.empty( ) )
      {
       char combinedUrl[1024];
       ParsedUrl parsedUrl( parser.url.cstr( ), true );
       parser.base = APESEARCH::string( parsedUrl.Service, parsedUrl.Port );
      } // end if

   //write to Nodes
   for(int i = 0; i < parser.links.size( ); ++i)
   {
      //Does it need a base tag ( has protocal ) and do we have it
      ParsedUrl parsedUrl( parser.links[i].URL.cstr() );
      if( strncmp( parser.links[i].URL.cstr(), "http", 4 ) )
      {
         APESEARCH::string rel = parser.links[i].URL;

         parser.links[i].URL = parser.base;
         parser.links[i].URL += rel;
      }
      if ( !strncmp( parser.links[i].URL.cstr(), "https", 5 ) )
            node.write(parser.links[i]);
      
   }
}

static std::string get_input ( );
void APESEARCH::Mercator::user_handler()
   {
    std::string input;
    do 
      {
      input = get_input( );
      switch( input.front() )
         {
         case 'I':
         case 'i':
            intel();
            break;
         case 'S': // Sum up pages crawled
         case 's': // Sum up pages crawled
            {
            size_t pagesCrawled = 0;
            for ( unsigned n = 0; n < SetOfUrls::maxPriority; ++n )
               {
               size_t num = queuesChosen[n].load( );
               std::cout << "N=" << n << ": " << num << '\n';
               pagesCrawled += num;
               } // end for
            std::cout << "Sum: " << pagesCrawled << '\n';
            break;
            }
         case 'R':
         case 'r':
            rate( );
            break;
         // Add here for more functionality
         case 'Q':
         case 'q':
            std::cout << "Attempting to exit elegantly\n";
            break;
         default:
            std::cerr << "Unrecognized command\n";
            break;
         } // end switch()
      }
    while( input.front() != 'Q' && input.front() != 'q' );

    // call cleanup handler
    cleanUp();
   }

// Keeps checking input stream and attempts to grab until it gets a string successfully.
static std::string get_input( )
   {
   std::string input;
   bool done = false;
   do
      {
      if ( std::cin >> input )
         {
         done = true;
         } // end if
      else // There's something wrong with the input stream
         {
         // std::cerr << "clearing cin stream\n" << std::endl;
         std::cin.clear( );
         // Try again once, And if it fails again, clear stream.
         //NOTE:The main reason for this is because when the user suspends the program,
         // this causes the cin stream to enter a fail state. However, prior to this change,
         // cin.ignore( ) itself was called, and since this is a blocking call AND there wasn't
         // any input, it required the input stream to consume additional characters before
         // being able to proceed. By accounting for such an edge case and assuming that the
         // stream was already empty, provides the user with an intuitive experience 
         // (which is typically the case when the user suspends the program).
         if ( std::cin >> input )
            {
            done = true;
            } // end if
         else // Still couldn't clear the input so we must assume it is invalid and clear the stream
            {
            // skip over the input until a newline is encountered or end-of-file is reached
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cerr << "done clearing cin stream\n" << std::endl;
            } // end else
         } // end else
      } while ( !done );
   return input;
   } // get_input( )

void APESEARCH::Mercator::intel()
   {
   APESEARCH::unique_lock<APESEARCH::mutex> lk( lkForCrawled );
   size_t *ptr = ( size_t * ) pagesCrawled.get();
   size_t num = *ptr;
   lk.unlock( );
   std::cout << "Pages Crawled: " << num << '\n';
   std::cout << "Crawled Since Launch: " << num - startCrawled << '\n';

   lk = APESEARCH::unique_lock<APESEARCH::mutex> ( lkForWritten );
   size_t totWritten = ptr[ 1 ];
   lk.unlock( );
   std::cout << "Pages Written To Disk: " << totWritten << '\n';
   std::cout << "Written Since Launch: " << totWritten - writtenAtStart << '\n';
   return;
   }

void APESEARCH::Mercator::rate( )
   {
   static std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
   static std::size_t crawledSinceLastCall = *( ( size_t * ) pagesCrawled.get() );
   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();

   APESEARCH::unique_lock<APESEARCH::mutex> lk(lkForCrawled);
   size_t getCurrCrawled = *( ( size_t * ) pagesCrawled.get() );
   lk.unlock( );
   auto startMs = std::chrono::time_point_cast<std::chrono::milliseconds>( start );
   auto endMs = std::chrono::time_point_cast<std::chrono::milliseconds>( end );
   auto startVal = startMs.time_since_epoch();
   auto endVal = endMs.time_since_epoch();
   long duration = endVal.count() - startVal.count();

   size_t diff = getCurrCrawled - crawledSinceLastCall;

   std::cout << "Milliseconds Elapsed: " << duration << '\n';
   std::cout << "Pages Crawled since last call: " << diff << '\n';
   if ( duration > 0 )
      std::cout << "Rate of Pages Crawled (page/min): " << ( double( diff ) / duration ) * 60000 << '\n';

   lk.lock( );
   crawledSinceLastCall = *( ( std::size_t * ) pagesCrawled.get() );
   start = std::chrono::system_clock::now();
   }

void APESEARCH::Mercator::cleanUp()
   {
    liveliness.store( false );
    frontier.shutdown( );
    std::cout << "Frontier finished cleaning up\n";
    pool.shutdown(); // Blocks until every thread has joined...
    return;
   } // end cleanUp()

void APESEARCH::Mercator::startUpCrawlers( const std::size_t amtOfCrawlers )
   {
   // Use indirection to work around member functions headache
   auto crawler = [this] ( )
      {
      this->crawler( );
      };
   // Start up crawlers
   for ( size_t n = 0; n < amtOfCrawlers; ++n )
      {
         pool.submitNoFuture(crawler);
      }
   } // end startUpCrawlers( )
