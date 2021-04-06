
#include "../include/crawler/Mercator.h"
//#include "../libraries/AS/include/AS/string.h"
#include <string> // needed to get idea out
#include <iostream>

// Only need one thread for this since it would otherwise 
// create contention...
void APESEARCH::Mercator::urlExtractor()
   {
    APESEARCH::string buffer;

   // C++ 14
    while( liveliness.load() )
       {
        buffer = urlBuffers.pop();

        frontier.getNextUrl( buffer ); // Writes directly to buffer
        
        // At the end of this task, the buffer will be reinserted back into urlBuffers...
        pool.submitNoFuture( &Mercator::getRequester, this, urlBuffers, std::move( buffer ) );
       } // end while
   } // end urlExtractor()

void APESEARCH::Mercator::getRequester( SharedQueue<  >&, APESEARCH::string&& url )
   {
   

   }

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

