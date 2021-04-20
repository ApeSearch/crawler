
#include "../include/crawler/Mercator.h"
#include "../Parser/HtmlParser.h"
//#include "../libraries/AS/include/AS/string.h"
#include <string> // needed to get idea out
#include <iostream>
#include <stdlib.h>     /* atoi */

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


void APESEARCH::Mercator::crawlWebsite( Request& requester, APESEARCH::string& buffer )
   {

   std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
   Result result = requester.getReqAndParse( buffer.cstr() );
   // At the end of this task, the buffer will be reinserted back into urlBuffers...
   std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();

   std::chrono::time_point<std::chrono::system_clock> whenCanCrawlAgain( getNewTime( start, end ) );
   ParsedUrl parsedUrl( buffer.cstr() );
   // This should be the case
   assert( *parsedUrl.Host ); 
   frontier.pool.submitNoFuture( [this, whenCanCrawlAgain{ std::move( whenCanCrawlAgain ) }, domain{ std::string( parsedUrl.Host, parsedUrl.Port ) } ](  ) 
   { this->frontier.backEnd.insertTiming( whenCanCrawlAgain, domain ); } );   

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
            APESEARCH::string url( result.url.begin(), result.url.end() );
            if( !bloomfilter.contains(url) )
               {
               Link link;
               link.URL = std::move( result.url );
               //TODO put on writer threadpool
               node.write( link );
               } // end if
            }
            break;
         default:
            break;
         } // end switch
   } // end crawlWebsite()

// Only need one thread for this since it would otherwise 
// create contention...
void APESEARCH::Mercator::crawler()
   {
    Request requester;
    string url;

    while( liveliness.load() )
       {
        url = frontier.getNextUrl( ); // Writes directly to buffer
        assert(!url.empty());
        //std::cerr << "ACTUALLY GOT A URL\n";
        crawlWebsite( requester, url );
       } // end while
   } // end urlExtractor()

void APESEARCH::Mercator::parser( const APESEARCH::vector< char >& buffer, const APESEARCH::string &url )
   {
   HtmlParser parser( buffer.begin( ), buffer.size(), url );

   // Handle results by writing to file...
   //TODO put this on own thread
   writeToFile( parser );
   std::cerr << "Crawled website successfully: " << url << '\n';
   APESEARCH::unique_lock<APESEARCH::mutex> lk(lkForPages);
   size_t *num = ( size_t * ) pagesCrawled.get();
   ++(*num);
   } // end parser()

void APESEARCH::Mercator::writeToFile( HtmlParser& parser )
{

   db.addParsedFile(parser);

   if(parser.base.empty())
      {
       char combinedUrl[1024];
       ParsedUrl parsedUrl( parser.url.cstr( ), true );
       parser.base = APESEARCH::string( parsedUrl.Service, parsedUrl.Port );
      } // end if

   //write to DB
   //write to Nodes
   for(int i = 0; i < parser.links.size(); ++i)
   {
      //Does it need a base tag ( has protocal ) and do we have it
      ParsedUrl parsedUrl( parser.links[i].URL.cstr() );
      if( strncmp( parser.links[i].URL.cstr(), "http", 4 ) )
      {
         APESEARCH::string rel = parser.links[i].URL;

         parser.links[i].URL = parser.base;
         parser.links[i].URL += rel;
      }
      
      node.write(parser.links[i]);
      
   }
}


void APESEARCH::Mercator::user_handler()
   {
    std::string input;
    do 
       {
        std::cin >> input; // Would need to overload operator>> for string here
        switch( input.front() )
           {
            case 'I':
                intel();
                break;
            // Add here for more functionality
            default:
                std::cerr << "Unrecognized command\n";
                break;
           } // end switch()
       }
    while( input.front() != 'Q' );

    // call cleanup handler
    cleanUp();
   }

void APESEARCH::Mercator::intel()
   {
   APESEARCH::unique_lock<APESEARCH::mutex> lk(lkForPages);
   size_t num = *( ( size_t * ) pagesCrawled.get() );

   std::cout << "Pages Crawled: " << num << std::endl;
   return;
   }

void APESEARCH::Mercator::cleanUp()
   {
    liveliness.store( false );
    pool.shutdown(); // Blocks until every thread has joined...
    return;
   } // end cleanUp()

void APESEARCH::Mercator::startUpCrawlers( const std::size_t amtOfCrawlers )
   {
   
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
