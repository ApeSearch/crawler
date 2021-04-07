#include <iostream>
#include <pthread.h>
#include <queue>
#include <string>
#include <set>

#include <cstring>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>

struct pthread_lock_guard {
     pthread_lock_guard(pthread_mutex_t& mutex) : _ref(mutex) { 
         pthread_mutex_lock(&_ref);  // TODO operating system specific
     };
     ~pthread_lock_guard() { 
         pthread_mutex_unlock(&_ref); // TODO operating system specific
     }
   private:
     pthread_mutex_t& _ref;
};

char cwd[PATH_MAX];
const int POOL_SIZE = 2;

pthread_mutex_t frontierLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t crawler_cv = PTHREAD_COND_INITIALIZER;

std::set<std::string> seen;
std::queue<std::string> frontier;

void* read_directory( void* args );

void print_directory( const std::set<std::string>& seen )
   {
   /* You may modify/not use this function, but make sure your output matches */
    for ( std::string s : seen )
      {
      if ( s.back( ) == '/' )
         s.pop_back( );
      std::cout << s << "\n";      
      }
   }

int main( int argc, char** argv )
   {
   if ( argc != 2 )
      {
      std::cerr << "Usage: ./listDirectory.exe <directory>" << std::endl;
      return 1;
      }
   getcwd(cwd, sizeof(cwd));
   /* Your main function goes here. This might be a good place to initialize your threads. 
    * This lab will not be graded for style. 
    * You must have a multithreaded program using pthreads.
    * You may use anything from the stl except stl threads.
    */
   
   pthread_t pool[POOL_SIZE];  
   frontier.push(std::string(argv[1]));
   for (int i = 0; i< POOL_SIZE; ++i) 
      {
         pthread_create(&(pool[i]), nullptr, &read_directory, (void *) i);
      }

   for (int i = 0; i< POOL_SIZE; ++i) 
      {
         pthread_join(pool[i], nullptr);
      }
   
   print_directory(seen);
   return 0;
   }

void* read_directory( void* args )
   {
   /* Your thread function goes here */
   std::string dirName;
   DIR *dir;
   struct dirent *dp;
   
   while (true) 
      {
         {
         auto g = pthread_lock_guard(frontierLock);
         // std::cout << (intptr_t) args << std::endl;
         while (frontier.size() == 0)
            {
            pthread_cond_wait(&crawler_cv, &frontierLock); 
            }
         
         dirName = frontier.front();
         frontier.pop();
         }
         
      if ((dir = opendir (dirName.c_str())) == NULL) {
         perror (dirName.c_str());
         continue;
      }
      
      auto g = pthread_lock_guard(frontierLock);
      while ((dp = readdir (dir)) != NULL) 
         {
         // Ignore '.' and '..' because we don't want to traverse literally everything
         if(strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
            {
            std::string newDirentry = dirName + "/" + dp->d_name;
            seen.insert(newDirentry);
            if (dp->d_type == DT_DIR)
               {
               frontier.push(newDirentry);
               pthread_cond_signal(&crawler_cv);
               }
            }
            std::cout << (intptr_t) args << " " << seen.size() << std::endl;
         }
      if (frontier.size() == 0) return nullptr;
      }
   }
