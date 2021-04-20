
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
#include "../../libraries/bloomFilter/include/bloomFilter/BloomFilter.h"
#include "Request.h"
#include "SetOfUrls.h"
#include "Node.h"
#include "DynamicBuffer.h"
#include <atomic>
#include <assert.h>
#include "ParsedUrl.h"

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

   using CircBuf = circular_buffer< Func, dynamicBuffer< Func > >;
    class Mercator
       {
      PThreadPool<CircBuf> pool; // The main threads that serve tasks
      SetOfUrls set;
      Database db;
      Node node;
      Bloomfilter bloomfilter;
      unique_mmap pagesCrawled;
      APESEARCH::mutex lkForPages;
      std::atomic<bool> liveliness; // Used to communicate liveliness of frontier
      //Node networkNode;

      void crawlWebsite( Request& requester, APESEARCH::string& buffer );
      void crawler();
      void parser( const APESEARCH::vector< char >& buffer, const APESEARCH::string &url );
      //TODO fix
      void writeToFile( HtmlParser& );
      //void getRequester( SharedQueue< APESEARCH::string >&, APESEARCH::string&& url );
      // Responsible for signaling and shutting down threads elegantly
      void intel();
      void cleanUp(); 
      void startUpCrawlers( const std::size_t );
    public:
      Mercator( APESEARCH::vector<APESEARCH::string>& ips, int id, const char *frontDir, const char * dbDir, size_t amtOfCrawlers, size_t amtOfParsers, size_t amtOfFWriters, APESEARCH::vector<Link>& seed_links ) : 
         // Size of buffer, amount of threads, max submits
         pool( CircBuf( ( amtOfCrawlers + amtOfParsers + amtOfFWriters ) * 3 ), amtOfCrawlers + amtOfParsers + amtOfFWriters, ( amtOfCrawlers + amtOfParsers + amtOfFWriters ) * 3 ) 
         ,set( frontDir ), db( dbDir ), bloomfilter(), node( ips, id, set, db, bloomfilter ), liveliness( true )
         {
         for (size_t i = 0; i < seed_links.size(); i++)
         {
            node.write(seed_links[i]);
         }
         
         APESEARCH::File file( "./pagesCrawledDONTTOUCH.txt", O_RDWR | O_CREAT, (mode_t) 0600 );
         std::cout << "Opened: /VirtualFileSystem/Root/pagesCrawledDONTTOUCH.txt" << std::endl;

         int fileSize = lseek(  file.getFD( ), 0, SEEK_END );
         if ( (long unsigned int)fileSize < sizeof( size_t ) )
            {
            ssize_t result = lseek( file.getFD( ), sizeof( size_t ) - 1, SEEK_SET );

            if ( result == -1 )
               {
               perror( "Issue with lseek while trying to stretch file" );
               return;
               } // end if
            
            result = write( file.getFD( ), "", 1 );

            if ( result == -1 )
               {
               perror( "Error writing bytes to file" );
               return;
               }
            } // end if
         pagesCrawled = unique_mmap( 0, sizeof( size_t ), PROT_READ | PROT_WRITE, MAP_SHARED, file.getFD( ) , 0 );

         startUpCrawlers( amtOfCrawlers );
         }
      ~Mercator();
      void run();
      void user_handler(); // Interacts with user to provide intel, look at stats, and to shutdown


      };
   } // end namesapce APESEARCH

std::chrono::time_point<std::chrono::system_clock> getNewTime( const std::chrono::time_point<std::chrono::system_clock>&, 
      const std::chrono::time_point<std::chrono::system_clock>&);



#endif
