//url -Hash---> machine, domain ---Hash2---> merc_queue
#pragma once

#ifndef NODE_H_AS
#define NODE_H_AS 

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/Socket.h"
#include "../../libraries/AS/include/AS/unique_ptr.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/condition_variable.h"
#include "../../libraries/AS/include/AS/as_semaphore.h"
#include "../../libraries/AS/include/AS/File.h"
#include "../../Parser/HtmlParser.h"
#include "../../libraries/HashTable/include/HashTable/FNV.h"
#include "../../libraries/AS/include/AS/pthread_pool.h"
#include "../../libraries/AS/include/AS/circular_buffer.h"
#include "../../libraries/bloomFilter/include/bloomFilter/BloomFilter.h"
#include "Frontier.h"
//#include "SetOfUrls.h"
#include "Database.h"
#include "DynamicBuffer.h"

struct NodeBucket
{
   NodeBucket( ) : writer_semaphore( 0 ) { }
   NodeBucket(size_t index, const char *ip);
   ~NodeBucket(){};
   NodeBucket& operator=( const NodeBucket  &other ) = delete;
   NodeBucket( const NodeBucket &other ) = delete;
   NodeBucket( NodeBucket &&other ) : addr( std::move( other.addr ) ), socket( std::move( other.socket ) )
   , storage_file( std::move( other.storage_file ) ), writer_semaphore(std::move(other.writer_semaphore) ) { }

   struct sockaddr_in addr;
   //Can probably use regular Socket variable if we implement move operator=
   APESEARCH::unique_ptr<Socket> socket; 
   APESEARCH::File storage_file;

   //Lock used for signaling that socket will be changed
   APESEARCH::mutex socket_lock;
   APESEARCH::condition_variable cv;
   APESEARCH::semaphore writer_semaphore;
   
   //Locks for file/socket IO
   APESEARCH::mutex L;
   APESEARCH::mutex M;
   APESEARCH::mutex N;

   void high_prio_lock()
   {
      N.lock();
      M.lock();
      N.unlock();
   }
   void high_prio_unlock()
   {
      M.unlock();
   }
   void low_prio_lock()
   {
      L.lock();
      N.lock();
      M.lock();
      N.unlock();
   }
   void low_prio_unlock()
   {
      M.unlock();
      L.unlock();
   }
};

class Node
{
   using CircBuf = APESEARCH::circular_buffer< APESEARCH::Func, APESEARCH::dynamicBuffer< APESEARCH::Func > >;
   APESEARCH::vector<NodeBucket> node_buckets;
   APESEARCH::string local_ip;
   int node_id;
   FNV hash;
   UrlFrontier& frontier;
   Bloomfilter bloomFilter;
   APESEARCH::mutex bloomFilter_lock;
   APESEARCH::PThreadPool< APESEARCH::circular_buffer<APESEARCH::Func, APESEARCH::DEFAULT::defaultBuffer<APESEARCH::Func, 32u>> > pool;
   Database& dataBase; 
   
   int retriesConnectAfterFailure( int, int );

public:
    //Try to connect to other nodes from ips
    //Start listening server
    //Check if swap files exist and how much data they have in them currently
    //Must have ips in some ordering!
    Node(APESEARCH::vector<APESEARCH::string> &ips, int node_id, UrlFrontier& _frontier, Database &db);
    ~Node();

    //1 dedicated thread-blocking
    void connectionHandler();

    //On functioncall stack
    //Try to send n times 
    //If cannot send write to local file
    //
    void write(Link &link);

    void sender(int index);

    // 7 dedicated threads non-blocking
    void receiver(int index);

    void connect(int index);
};

#endif