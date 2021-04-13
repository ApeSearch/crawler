
#include "../include/crawler/SetOfUrls.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include "../libraries/AS/include/AS/File.h"
#include "../libraries/AS/include/AS/algorithms.h" // for APESEARCH::copy
#include <cstring> // for strlen
#include <unistd.h> // for getcwd
#include <stdlib.h> // for mkstemp
#include <stdio.h> // for unlink() => remove paths
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
 * REQUIRES: filename is a valid file
 * MODIFIES: Nothing
 *  EFFECTS: Verifies that a specific file is one that is part of the frontier
 *           by checking the last character ( which should be '\r' ).
*/
bool SetOfUrls::verifyFile( const char *filename ) const
   {
   char buf[1024];
   snprintf( buf, sizeof( buf ), "%s%c%s", dirPath, '/', filename );
   APESEARCH::File temp( buf, O_RDONLY );
   ssize_t result = lseek( temp.getFD(), off_t( FileSize( temp.getFD() ) - 1 ), SEEK_SET );
   read( temp.getFD(), buf, 1024 );
   return *buf == '\r';
   }

bool SetOfUrls::forceWrite()
   {
   bool retval = numOfUrlsInserted.load() > 0;
   if ( retval )
      {
      APESEARCH::unique_lock<APESEARCH::mutex> bQLk( backQLk );
      finalizeSection(); // Forcefully write to file
      } // end if

   return retval;
   }

SetOfUrls::SetOfUrls() : SetOfUrls( SetOfUrls::frontierLoc )
   {
   }


SetOfUrls::SetOfUrls( const char *directory ) : frontQPtr( nullptr ), numOfUrlsInserted( 0 )
   {
   assert( *directory == '/' );
   getcwd( cwd, PATH_MAX );

   // Copy name into a buffer so it can be used later to close and reopen
   snprintf( dirPath, sizeof( dirPath ), "%s%s", cwd, directory );
   if ( ( dir = opendir( dirPath ) ) == NULL )
      {
      perror( dirPath );
      throw std::runtime_error( "VirtualFileSytem couldn't be opened" );
      } // end if
   

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
#ifdef DEBUG
   std::cerr << "Running Destructor...\n";
#endif
   if ( numOfUrlsInserted )
      {
      APESEARCH::unique_lock<APESEARCH::mutex> lk( backQLk );
      finalizeSection();
      }
   // Write the rest of front of queue into a different place...
   if ( frontQPtr )
      {
      // Looks for a valid url...
      if ( APESEARCH::find( frontQPtr, frontQEnd, '\n' ) != frontQEnd )
         {
         ++numOfUrlsInserted;
      {
         APESEARCH::unique_lock<APESEARCH::mutex> lk( backQLk );
         startNewFile();
      }
         back.write( frontQPtr, frontQEnd - frontQPtr );
      {
         APESEARCH::unique_lock<APESEARCH::mutex> lk( backQLk );
         finalizeSection();
      }        
         } // end if
         assert( removeFile( frontQFileName ) );
      } // end if
   removeFile( backQPath );
   closedir( dir );
   } // end destructor

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
   do
   {
      APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
      while( (dp = readdir (dir)) != NULL )
         {
         if ( dp->d_type == DT_REG && !strncmp( "urlSlice", dp->d_name, 8 ) )
            {
            if ( verifyFile( dp->d_name ) )
               break;
            else // Possible something bad happened to this file
               {
               char filename[1024];
               snprintf( filename, sizeof(filename), "%s%c%s", dirPath, '/', dp->d_name );
               removeFile( filename );
               rewinddir( dir );
               }
            }
         } // end while
   } while ( forceWrite() );
   return dp;
   }



// Assumes lock is held
void SetOfUrls::startNewFile()
   {
   assert( !backQLk.try_lock() );
   //char filename[] = "/tmp/temp.XXXXXX";
   snprintf( backQPath, sizeof( backQPath ), "%s%c%s", dirPath, '/', "temp.XXXXXX" );
   //snprintf( backQPath, sizeof( backQPath ), "%s", "temp.XXXXXX" );
   // Open a new temp file
   int fd = mkstemp( backQPath );
   if ( fd < 0 )
      {
      std::cerr << "Issue making a temporary file\n" << std::endl;
      throw std::runtime_error( "Issue making a temporary file\n");
      }
   back = APESEARCH::File( fd );
#ifdef DEBUG
   fprintf( stderr, "Starting new file%s\n", backQPath );
#endif
   } // end startNewFile()

bool SetOfUrls::removeFile( const char *fileName )
   {
   assert( fileName );
#ifdef DEBUG
   fprintf( stderr, "Removing file %s from disk\n", fileName );
#endif
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
   assert( !backQLk.try_lock() );
   assert( numOfUrlsInserted.load() > 0 );

   // Write to help signify this is a right file...
   write( back.getFD(), "\r", 1 );

   static std::size_t globalCnt = 0;
   assert( !backQLk.try_lock() );
   char finalPath[PATH_MAX];

   std::chrono::time_point<std::chrono::system_clock> timeNow = std::chrono::system_clock::now();
   std::time_t converted = std::chrono::system_clock::to_time_t( timeNow );
   snprintf( finalPath, sizeof( finalPath ), "%s%c%s%zu%s", dirPath, '/', "urlSlice" , globalCnt++, std::ctime( &converted ) );
   APESEARCH::replace( finalPath, finalPath + strlen( finalPath ), ' ', '-' ); // replace spaces with -
   APESEARCH::replace( finalPath, finalPath + strlen( finalPath ), '\n', '\0' ); 

   if ( rename( backQPath, finalPath ) < 0 )
      {
      perror("Issue with finalizeSection:");
      throw std::runtime_error( "Issue with finalizeSection" );
      }  // end if
#ifdef DEBUG
   fprintf( stderr, "File temp has now been renamed from %s to %s\n", backQPath, finalPath );
   fprintf( stderr, "Written %s to disk\n", finalPath );
#endif

   numOfUrlsInserted.store(0);
   
   // Grab lock for DIR
   // Dir entries have now been changed
   APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
   rewinddir( dir );

   // Need to start new file while dir lock is held
   startNewFile();
   }

static unsigned calcPriority( const APESEARCH::string& )
   {
   return 0;
   }

inline UrlObj SetOfUrls::helperDeq()
   {
   assert( !frntQLk.try_lock() );

   // Ensure that frontQPtr isn't frontQEnd...
   assert( !frontQPtr || frontQPtr != frontQEnd );

   if ( !frontQPtr && !popNewBatch() )
       return UrlObj(); // Frontier is empty

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
         rewinddir( dir );
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
   obj.priority = calcPriority( obj.url ); // All have a priority of 0
   
   // Increment pointer now that url has been copied
   if ( ++frontQPtr == frontQEnd )
      {
      assert( removeFile( frontQFileName ) );
   {
      APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
      rewinddir( dir );
   }
      frontQPtr = nullptr;
      popNewBatch();
      }
   return obj;
   }

// Front -> back -> directory lock (possible)
UrlObj SetOfUrls::dequeue()
   {
   APESEARCH::unique_lock<APESEARCH::mutex> uniqLk( frntQLk );
   return helperDeq();
   }

// Front -> back -> directory lock (possible)
UrlObj SetOfUrls::blockingDequeue()
   {
   UrlObj url;
   APESEARCH::unique_lock<APESEARCH::mutex> uniqLk( frntQLk );
   do
   {
      cv.wait( uniqLk, [this]( ){ return frontQPtr || popNewBatch(); } );
      url = helperDeq();
   } while ( url.url.empty() );
   
   return url;
   }

const char *SetOfUrls::front( )
   {
   APESEARCH::unique_lock<APESEARCH::mutex> uniqLk( frntQLk );
   assert( !frontQPtr || frontQPtr != frontQEnd );

   if ( !frontQPtr && !popNewBatch() )
      return nullptr;

   char character;
   char const *start = frontQPtr;
   char const *ptr = frontQPtr;
   while ( ( character = *ptr ) != '\n' )
      {
      // Finished with file and now has to go to new one...
      if ( ++ptr == frontQEnd )
         {
         assert( removeFile( frontQFileName ) );
      {
         APESEARCH::unique_lock<APESEARCH::mutex> lk( dirLk );
         rewinddir( dir );
      }
         frontQPtr = nullptr;
         if ( popNewBatch() )
            start = ptr = frontQPtr;
         else
            return nullptr;
         } // end if
      } // end while
   return start;
   }


void SetOfUrls::enqueue( const APESEARCH::string &url )
   {
   APESEARCH::unique_lock<APESEARCH::mutex> lk( backQLk );
   assert( numOfUrlsInserted.load() < SetOfUrls::maxUrls && !url.empty() );

   write( back.getFD(), url.cbegin(), url.size() );

   // Mark as a delimiter
   write( back.getFD(), "\n", 1 );
   ++numOfUrlsInserted;
   cv.notify_one(); // Notify any potentially waiting threads

   if ( numOfUrlsInserted == SetOfUrls::maxUrls )
      {
      finalizeSection(); // assumes backQLk is held
      } // end if
   } // end enqueue()

