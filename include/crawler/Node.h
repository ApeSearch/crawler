//url -Hash---> machine, domain ---Hash2---> merc_queue
#pragma once

#ifndef NODE_H_AS
#define NODE_H_AS 

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/Socket.h"
#include "../../libraries/AS/include/AS/unique_ptr.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/File.h"
#include "../../Parser/HtmlParser.h"
#include "../../libraries/HashTable/include/HashTable/FNV.h"
#include "../../libraries/AS/include/AS/pthread_pool.h"
#include "../../libraries/AS/include/AS/circular_buffer.h"
#include "../../libraries/bloomFilter/include/bloomFilter/BloomFilter.h"
#include "SetOfUrls.h"
#include "Database.h"
#include "DynamicBuffer.h"

class Node
{
   using CircBuf = APESEARCH::circular_buffer< APESEARCH::Func, APESEARCH::dynamicBuffer< APESEARCH::Func > >;
   APESEARCH::vector<struct sockaddr_in> addrinfos;
   APESEARCH::vector<APESEARCH::unique_ptr<Socket>> sockets;
   APESEARCH::vector<APESEARCH::File> storage_files;
   APESEARCH::vector<APESEARCH::mutex> locks; //Used for sends, and atomic updates to storage_files
   APESEARCH::string local_ip;
   int node_id;
   FNV hash;
   SetOfUrls& set;
   Bloomfilter bloomFilter;
   APESEARCH::mutex bloomFilter_lock;
   APESEARCH::PThreadPool< APESEARCH::circular_buffer<APESEARCH::Func, APESEARCH::DEFAULT::defaultBuffer<APESEARCH::Func, 32u>> > pool;
   Database& dataBase; 
    

public:
    //Try to connect to other nodes from ips
    //Start listening server
    //Check if swap files exist and how much data they have in them currently
    //Must have ips in some ordering!
    Node(APESEARCH::vector<APESEARCH::string> &ips, APESEARCH::string &loc_ip, SetOfUrls& _set, Database &db);
    ~Node();

    //1 dedicated thread-blocking
    void connectionHandler();

    //On functioncall stack
    //Try to send n times 
    //If cannot send write to local file
    //
    void send(Link &link);

    void sender(int i);

    // 7 dedicated threads non-blocking
    void receive(int i);

    void connector(int i);
};

#endif