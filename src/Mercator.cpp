
#include "../include/crawler/Mercator.h"
#include "../Parser/HtmlParser.h"
//#include "../libraries/AS/include/AS/string.h"
#include <string> // needed to get idea out
#include <iostream>

// Only need one thread for this since it would otherwise 
// create contention...
void APESEARCH::Mercator::crawler()
   {
    Request requester;
    APESEARCH::string buffer;
    Result result;

    while( liveliness.load() )
       {
        frontier.getNextUrl( buffer ); // Writes directly to buffer
        
        result = requester.getReqAndParse( buffer.c_str() );

        // At the end of this task, the buffer will be reinserted back into urlBuffers...
        switch( result.status )
           {
            case getReqStatus::successful:
               pool.submitNoFuture( &Mercator::parser, this, std::move( result ), 
                  std::move( requester.getResponseBuffer.first() ) );
               break;
            case getReqStatus::redirected:
               frontier.insertNewUrl( std::move( result.url ) );
               break;
            default:
               break;
           } // end switch
       } // end while
   } // end urlExtractor()

void APESEARCH::Mercator::parser( std::string&& buffer )
   {
   HtmlParser parser( buffer.c_str(), buffer.size() );

   // Handle results by writing to file...
   writeToFile( parser );
   } // end parser()

void APESEARCH::Mercator::user_handler()
   {
    std::string input;
    do 
       {
        cin >> input; // Would need to overload operator>> for string here
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

void APESEARCH::MERACTOR::cleanUp()
   {
    liveliness.store( false );
    pool.shutdown(); // Blocks until every thread has joined...
    return;
   } // end cleanUp()

