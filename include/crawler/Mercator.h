
#pragma once
#ifndef AS_MERCATOR_H
#define AS_MERCATOR_H

#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/pthread_pool.h"
#include "Crawler.h"
#include "Request.h"
#include "Frontier.h"

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

    class Mercator
       {
    PThreadPool pool; // The main threads that serve tasks
    APESEARCH::vector< Request > requesters;
    UrlFrontier frontier;
    
    public:




       };
   } // end namesapce APESEARCH



#endif