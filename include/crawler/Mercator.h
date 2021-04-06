
#pragma once
#ifndef AS_MERCATOR_H
#define AS_MERCATOR_H

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/pthread_pool.h"
#include "Crawler.h"
#include "Request.h"
#include "Frontier.h"
#include "../../libraries/AS/include/AS/atomic_queue.h"
#include "../../libraries/AS/include/AS/condition_variable.h"
#include "../../libraries/AS/include/AS/as_semaphore.h"
#include "../../libraries/AS/include/AS/circular_buffer.h"
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
   
   template< typename T>
    class dynamicBuffer : public Buffer<T>
       {
       vector<T> buffer;
       typedef T value_type;
       
       dynamicBuffer( const size_t capacity, T&val ) : buffer( capacity, val )
          {
          assert( computeTwosPowCeiling( capacity ) == capacity );
          }
       inline value_type *getBuffer() noexcept
          {
          return &buffer.front();
          }
       inline void insert( const T& val, size_t index ) noexcept
          {
          buffer[ val ] = val;
          }
       inline void insert( sizie_t index, T&& val ) noexcept
          {
          buffer[ val ] = std::forward<T>( val );
          }
       inline virtual T& get( size_t )
          {
          return buffer[ val ];
          }
       
       inline virtual size_t getCapacity() const
          {
          return buffer.size();
          }
       inline virtual T *begin() noexcept
          {
          return &buffer.front();
          }
       void print( std::ostream& os, const size_t , const size_t sizeOf ) const
          {
          return;
          }
       };

    /*
     * Objct used to encapsulate a consumer queue ( the producer queue is the thread pool itself )
    */
    template< typename T >
    class SharedQueue
       {
       atomic_queue< T, circular_buffer< T, dynamicBuffer< T > > queue;
       semaphore available;
      
       public:
         SharedQueue( std::size_t amountOfResources, T& val ) : 
            queue( circular_buffer< T, 
            dynamicBuffer< T > >( amountOfResouces, val ) ) {}
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


    class Mercator
       {
      PThreadPool pool; // The main threads that serve tasks
      UrlFrontier frontier;
      std::atomic<bool> liveliness; // Used to communicate liveliness of frontier


      void crawler();
      void parser( std::string&& buffer );
      void writeToFile( HtmlParser& );
      //void getRequester( SharedQueue< APESEARCH::string >&, APESEARCH::string&& url );
      // Responsible for signaling and shutting down threads elegantly
      void cleanUp(); 
    public:
      Mercator( size_t amtOfUrlsSubmitters, size_t amtOfParsers, size_t amtOfFileWriters ) : 
         pool( amtOfUrlsSubmitters + amtOfParsers + amtOfFileWriters ) : liveliness( true ) {}
      void run();
      void user_handler(); // Interacts with user to provide intel, look at stats, and to shutdown


      };
   } // end namesapce APESEARCH



#endif