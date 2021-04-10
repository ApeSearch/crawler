
#include "../include/crawler/SetOfUrls.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include "../libraries/AS/include/AS/File.h"
#include "../libraries/AS/include/AS/algorithms.h" // for APESEARCH::copy
#include <cstring> // for strlen
#include <unistd.h> // for getcwd
#include <stdlib.h> // for mkstemp
#include <stdio.h> // for unlink() => remove paths
#include <stdio.h>
#include <chrono>
#include <ctime>

//--------------------------------------------------------------------------------
//
//
//                   Miscellaneous Helper functions
//                
//
//--------------------------------------------------------------------------------

/*
 *  Gets the file size of an open file.
*/
static size_t FileSize( int f )
   {
   struct stat fileInfo;
   fstat( f, &fileInfo );
   return ( size_t )fileInfo.st_size;
   }
      
/*
 * Shrinks a char vector to newSize
*/
static void shrinkSize( APESEARCH::vector<char>& buffer, const size_t newSize )
   {
   while( newSize < buffer.size() )
      buffer.pop_back();
   }

/*
 *  Searches a Directory ( dir ) until it either reaches the end ( when readdir returns NULL),
 *  while skipping any non-files and the . and .. directories.
 * 
 * REQUIRES: Nothing
 * MODIFIES: DIR *dir (through readdir and finalizeSection), back ( through finalizeSection->startNewFile ),
 *           numOfUrlsInserted ( through finalizeSection )
 *  EFFECTS: Searches through DIR *dir's direntries for a file that can be memory-mapped for urls.
 *           Returns a pointer to it, else returns an empty one ( where direntry * is a nullptr )
*/
struct dirent *SetOfUrls::getNextDirEntry( DIR *dir )
   {
   struct dirent *dp;
{
   APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
   while( (dp = readdir (dir)) != NULL &&  dp->d_type != DT_REG );
}
   if ( numOfUrlsInserted.load() )
      {
   {
      APESEARCH::unique_lock<APESEARCH::mutex> bQLk( backQLk );
      finalizeSection();
   } // release bQLk
   { // Regrabs dirLk to grab the direent
      APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
      while( (dp = readdir (dir)) != NULL &&  dp->d_type != DT_REG )
         {
         printf("%s\n", dp->d_name);
         }
      assert( dp != NULL );
   }
      }
   return dp;
   }


SetOfUrls::SetOfUrls() : SetOfUrls( SetOfUrls::frontierLoc )
   {
   }


SetOfUrls::SetOfUrls( const char *directory ) : frontQPtr( nullptr ), numOfUrlsInserted( 0 )
   {
   assert( *directory == '/' );
   getcwd( cwd, PATH_MAX );

   snprintf( dirPath, sizeof( dirPath ), "%s%s", cwd, directory );
   if ( ( dir = opendir( dirPath ) ) == NULL )
      {
      perror( dirPath );
      throw std::runtime_error( "VirtualFileSytem couldn't be opened" );
      } // end if
   
   // Copy name into a buffer so it can be used later to close and reopen

   if ( !popNewBatch() )
      {
      // Signify an empty frontier
      frontQPtr = nullptr;
      } // end else
   
   // Open a temporary file to write into...
   APESEARCH::unique_lock<APESEARCH::mutex> lk( backQLk );
   startNewFile();
   }

SetOfUrls::~SetOfUrls()
   {
   std::cout << "Running Destructor...\n";
   if ( numOfUrlsInserted )
      {
      finalizeSection();
      }
   assert( removeFile( backQPath ) );
   closedir( dir );
   }

// Assumes lock is held
void SetOfUrls::startNewFile()
   {
   assert( !backQLk.try_lock() );
   //char filename[] = "/tmp/temp.XXXXXX";
   snprintf( backQPath, sizeof( backQPath ), "%s", "temp.XXXXXX" );
   // Open a new temp file
   int fd = mkstemp( backQPath );
   if ( fd < 0 )
      {
      std::cerr << "Issue making a temporary file\n" << std::endl;
      throw std::runtime_error( "Issue making a temporary file\n");
      }
   back = APESEARCH::File( fd );

   fprintf( stderr, "Starting new file%s\n", backQPath );
   } // end startNewFile()

bool SetOfUrls::removeFile( const char *fileName )
   {
   assert( fileName );
   fprintf( stderr, "Removing file %s from disk\n", fileName );
   return !remove( fileName );
   }

/*
 * REQUIRES: Nothing
 *  MODIFES: frontOfQueue ( opening a new file in the frontier directory )
 *           and points frontQPtr to the start of it memory mapped.
 *  EFFECTS: Iterates through the Frontier directory to find a new section of
 *           urls to crawl. If getNextDirEntry can't find a directory entry to
 *           open, returns false.
*/
bool SetOfUrls::popNewBatch()
   {
   assert( !frontQPtr );
   struct dirent *dp;
   if ( ( dp = getNextDirEntry( dir ) ) )
      {
      // Open and memory map it
      snprintf( frontQFileName, sizeof ( frontQFileName ), "%s%c%s", dirPath, '/', dp->d_name );
      APESEARCH::File file( frontQFileName, O_RDONLY );
      const int fd = file.getFD();
      std::size_t fileSize = FileSize( fd );

      // initialize new state
      frontOfQueue = unique_mmap( 0, fileSize, PROT_READ, MAP_PRIVATE, fd, 0 );
      frontQPtr = reinterpret_cast< char *>( frontOfQueue.get() );
      frontQEnd = frontQPtr + fileSize;
      } // end if
   return dp;
   } // end popNewBack()

// assumes backQLk is acquired and acquires directory lock
// Called in getNextDirEnt
// and enqueue
void SetOfUrls::finalizeSection( )
   {
   static std::size_t globalCnt = 0;
   assert( !backQLk.try_lock() );
   char finalPath[PATH_MAX];
   char sysTime[1024];
   std::chrono::time_point<std::chrono::system_clock> timeNow = std::chrono::system_clock::now();
   std::time_t converted = std::chrono::system_clock::to_time_t( timeNow );
   snprintf( sysTime, sizeof( sysTime ), "%s%zu%s", "urlSlice" , globalCnt++, std::ctime( &converted ) );
   APESEARCH::replace( sysTime, sysTime + strlen( sysTime ), ' ', '-' ); // replace spaces with -
   //snprintf( finalPath, sizeof( finalPath ), "%s%s%c", dirPath, sysTime, '\0' );

   // Create a hard link
   if ( linkat( AT_FDCWD, backQPath, dir->__dd_fd, sysTime, 0 ) < 0 )
      {
      perror("Issue with finalizeSection:");
      throw std::runtime_error( "Issue with finalizeSection" );
      }  // end if
   fprintf( stderr, "Written %s to disk\n", finalPath );

   numOfUrlsInserted.store(0);
   
   // Remove temporary file
   assert( removeFile( backQPath ) );

   // Grab lock for DIR
   // Dir entries have now been changed
   APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
   closedir( dir );
   dir = opendir( dirPath );
   if ( dir == NULL )
      {
      perror( dirPath );
      throw std::runtime_error( "VirtualFileSytem couldn't be opened" );
      } // end if

   // Need to start new file while dir lock is held
   startNewFile();
   }


// Front -> back -> directory lock (possible)
UrlObj SetOfUrls::dequeue()
   {
   // Ensure that frontQPtr isn't frontQEnd...
   APESEARCH::unique_lock<APESEARCH::mutex> uniqLk( frntQLk );
   if ( frontQPtr )
      assert( frontQPtr != frontQEnd );
   if ( !frontQPtr && !popNewBatch() )
      {
      frontQPtr = nullptr;
       return UrlObj(); // Frontier is empty
      }
   char character;
   char const *start = frontQPtr;
   while ( ( character = *frontQPtr ) != '\n' )
      {
      // Finished with file and now has to go to new one...
      if ( ++frontQPtr == frontQEnd )
         {
         assert( removeFile( frontQFileName ) );
      {
         APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
         closedir( dir );
         dir = opendir( dirPath );
         if ( dir == NULL )
            {
            perror( dirPath );
            throw std::runtime_error( "VirtualFileSytem couldn't be opened" );
            } // end if
      }
         frontQPtr = nullptr;
         if ( popNewBatch() )
            start = frontQPtr;
         else
            return UrlObj(); // Frontier is empty
         } // end if
      } // end while

   UrlObj obj;
   assert( start && frontQPtr && start < frontQPtr );
   obj.url = APESEARCH::string( start, frontQPtr );
   obj.priority = 69;

   // Increment pointer now that url has been copied
   if ( ++frontQPtr == frontQEnd )
      {
      assert( removeFile( frontQFileName ) );
   {
      APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
      closedir( dir );
      dir = opendir( dirPath );
      if ( dir == NULL )
         {
         perror( dirPath );
         throw std::runtime_error( "VirtualFileSytem couldn't be opened" );
         } // end if
   }
      frontQPtr = nullptr;
      popNewBatch();
      }
   return obj;
   }

void SetOfUrls::enqueue( APESEARCH::string &&url )
   {
   enqueue( url );
   }

void SetOfUrls::enqueue( const APESEARCH::string &url )
   {
   APESEARCH::unique_lock<APESEARCH::mutex> lk( backQLk );
   assert( numOfUrlsInserted.load() < SetOfUrls::maxUrls && !url.empty() );

   write( back.getFD(), url.cbegin(), url.size() );

   // Mark as a delimiter
   write( back.getFD(), "\n", 1 );
   ++numOfUrlsInserted;

   if ( numOfUrlsInserted == SetOfUrls::maxUrls )
      {
      finalizeSection(); // assumes backQLk is held
      } // end if
   } // end enqueue()

