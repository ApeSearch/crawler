
#pragma once
#ifndef AS_MERCATOR_H
#define AS_MERCATOR_H

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/pthread_pool.h"
#include "../../libraries/AS/include/AS/atomic_queue.h"
#include "../../libraries/AS/include/AS/condition_variable.h"
#include "../../libraries/AS/include/AS/as_semaphore.h"
#include "../../Parser/HtmlParser.h"
#include "Request.h"
#include "Frontier.h"
#include "Node.h"
#include "DynamicBuffer.h"
#include <atomic>
#include <assert.h>

/*
 * The general flow of this design is to seperate out
 * different parts of the crawler into smaller tasks
 * that work on a small portion of work.
 * 
 * For example, one thread will dedicate popping from the frontier
 * and setting up a task for the thread pool 
 * ( via getting the actual html ).
 * The finishing thread will basically send the results through
 * a promise object which will take a buffer and parse it.
 * 
 * Afterwards, 
 * 
*/
namespace APESEARCH
   {
   
    /*
     * Objct used to encapsulate a consumer queue ( the producer queue is the thread pool itself )
    */
   /*
    template< typename T >
    class SharedQueue
       {
       atomic_queue< T, circular_buffer< T, dynamicBuffer< T > > > queue;
       semaphore available;
      
       public:
         SharedQueue( std::size_t amountOfResources, T& val ) : 
            queue( circular_buffer< T,  
            dynamicBuffer< T > >( amountOfResources, val ) ) {}
         ~SharedQueue( ) {}

       void push( T&& val ) noexcept
          {
           queue.enqueue( std::forward<T>( val ) );
           available.up();
          }

       T&& pop( ) noexcept
          {
           available.down();
           return queue.dequeue().value();
          }
       };
   */

   using CircBuf = circular_buffer< Func, dynamicBuffer< Func > >;
    class Mercator
       {
      PThreadPool<CircBuf> pool; // The main threads that serve tasks
      UrlFrontier frontier;
      std::atomic<bool> liveliness; // Used to communicate liveliness of frontier
      //Node networkNode;

      void crawlWebsite( APESEARCH::string& buffer );
      void crawler();
      void parser( const std::string& buffer, const APESEARCH::string &url );
      void parser(  );
      void writeToFile( const HtmlParser& );
      //void getRequester( SharedQueue< APESEARCH::string >&, APESEARCH::string&& url );
      // Responsible for signaling and shutting down threads elegantly
      void intel();
      void cleanUp(); 
    public:
      Mercator( size_t amtOfCrawlers, size_t amtOfParsers, size_t amtOfFWriters ) : 
         pool( CircBuf( amtOfCrawlers + amtOfParsers + amtOfFWriters ), 
         amtOfCrawlers + amtOfParsers + amtOfFWriters, ( amtOfCrawlers + amtOfParsers + amtOfFWriters ) * 2 ), 
         frontier( amtOfCrawlers ), liveliness( true ) {}
      ~Mercator();
      void run();
      void user_handler(); // Interacts with user to provide intel, look at stats, and to shutdown


      };
   } // end namesapce APESEARCH

std::chrono::time_point<std::chrono::system_clock> getNewTime( const std::chrono::time_point<std::chrono::system_clock>&, 
      const std::chrono::time_point<std::chrono::system_clock>&);



#endif