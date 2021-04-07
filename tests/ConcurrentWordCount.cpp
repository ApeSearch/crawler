#include <iostream>
#include <pthread.h>

#include <sstream>
#include <fstream>
#include <string>

// #include "helpers.h"

pthread_mutex_t countLock = PTHREAD_MUTEX_INITIALIZER;
int count;

void* word_count( void* args );

void print_word_count( int count )
   {
   /* You may modify/not use this function, but make sure your output matches */
   std::cout << "Total Words: " << count << std::endl;
   }

int main( int argc, char** argv )
   {
   if ( argc <= 1 )
      {
      std::cerr << "Usage: ./wordCount.exe <files>" << std::endl;
      return 1;
      }
   
   /* Your main function goes here. This might be a good place to initialize your threads. 
    * This lab will not be graded for style. 
    * You must have a multithreaded program using pthreads.
    * You may use anything from the stl except stl threads.
    */
   pthread_t pool[argc-1];
   for (int i = 1; i < argc; ++i)
      {
      pthread_create(&(pool[i-1]), NULL, &word_count,(void *) argv[i]);
      }
   
   for (int i = 1; i < argc; ++i)
      {
      pthread_join(pool[i-1], NULL);
      }

   print_word_count(count);
   return 0;
   }

void* word_count( void* args )
   {
   /* Your thread function goes here */
   std::ifstream fileToCount((const char*) args);
   
   if (fileToCount.is_open())
      {
         int wc = 0;
         std::string word;
         while (fileToCount >> word) 
            ++wc;
         pthread_mutex_lock(&countLock);
         count += wc;
         pthread_mutex_unlock(&countLock);
      }
      fileToCount.close();
   }