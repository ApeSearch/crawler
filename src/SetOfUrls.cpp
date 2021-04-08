
#include "../include/crawler/SetOfUrls.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include "../libraries/AS/include/AS/File.h"
#include "../libraries/AS/include/AS/algorithms.h" // for APESEARCH::copy
#include <cstring> // for strlen
#include <unistd.h> // for getcwd
#include <stdlib.h> // for mkstemp

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
*/
struct dirent *getNextDirEntry( DIR *dir )
   {
   struct dirent *dp;
   while( (dp = readdir (dir)) != NULL &&  dp->d_type != DT_REG );
   return dp;
   }


SetOfUrls::SetOfUrls() : SetOfUrls( SetOfUrls::frontierLoc )
   {
   }


SetOfUrls::SetOfUrls( const char *directory ) : numOfUrlsInserted( 0 )
   {
   getcwd( cwd, PATH_MAX );
   cwdLength = strlen( cwd );
   assert(  cwdLength <= PATH_MAX );
   //shrinkSize( cwd, strlen( cwd.begin() ) );

   if ( ( dir = opendir( directory ) ) == NULL )
      {
      perror( directory );
      throw std::runtime_error( "VirtualFileSytem couldn't be opened" );
      } // end if
   
   // Skip until a valid file has been reached
   size_t lengthOfDir = strlen( directory );
   cwd[ cwdLength++ ] = '/';
   APESEARCH::copy( directory, directory + lengthOfDir, cwd + cwdLength );
   cwdLength += lengthOfDir;
   cwd[ cwdLength++ ] = '/';
   cwd[ cwdLength ] = '\0'; // for safetly

   if ( !popNewBatch() )
      {
      // Signify an empty frontier
      frontQPtr = nullptr; 
      } // end else
   
   // Open a temporary file to write into...
   startNewFile();
   }

SetOfUrls::~SetOfUrls()
   {
   closedir( dir );
   }

void SetOfUrls::startNewFile()
   {
   //char filename[] = "/tmp/temp.XXXXXX";
   char filename[] = "temp.XXXXXX"; // For testing...
   // Open a new temp file
   int fd = mkstemp( filename );
   if ( fd < 0 )
      {
      std::cerr << "Issue making a temporary file\n" << std::endl;
      throw std::runtime_error( "Issue making a temporary file\n");
      }
   back = APESEARCH::File( fd );

   }

/*
 * REQUIRES: Nothing
 *  MODIFES: frontOfQueue ( opening a new file in the frontier directory )
 *           and points frontQPtr to the start of it memory mapped.
 *  EFFECTS: Iterates through the Frontier directory to find a new section of
 *           urls to crawl.
*/
bool SetOfUrls::popNewBatch()
   {
   struct dirent *dp;
   char buf[PATH_MAX];
   if ( ( dp = getNextDirEntry( dir ) ) )
      {
      snprintf(buf, sizeof buf, "%s%s", cwd, dp->d_name );
      APESEARCH::File file( buf, O_RDONLY );
      cwd[ cwdLength ] = '\0';
      int fd = file.getFD();
      size_t fileSize = FileSize( fd );
      frontOfQueue = unique_mmap( 0, fileSize, PROT_READ, MAP_PRIVATE, fd, 0 );
      frontQPtr = reinterpret_cast< char *>( frontOfQueue.get() );
      frontQEnd = frontQPtr + fileSize;
      } // end if
   else
      frontQPtr = nullptr;
   return dp;
   } // end popNewBack()

void SetOfUrls::finalizeSection( )
   {
   static unsigned id = 0;

   //cwd.push_back( id++ ); // Add a unique identifier
   //cwd.push_back( 0 ); // Add null-character
   // Create a hard link
   linkat( back.getFD(), NULL, AT_FDCWD, cwd, 0 ); 

   // Modifies
   startNewFile();
   }


UrlObj SetOfUrls::dequeue()
   {
   // Ensure that frontQPtr isn't frontQEnd...
   assert( frontQPtr != frontQEnd );
   if ( !frontQPtr && !popNewBatch() )
      {
       return UrlObj(); // Frontier is empty
      }
   char character;
   char *start = frontQPtr;
   while ( ( character = *frontQPtr ) != '\n' )
      {
      // Finished with file and now has to go to new one...
      if ( ++frontQPtr == frontQEnd )
         {
         if ( popNewBatch() )
            start = frontQPtr;
         else
            return UrlObj(); // Frontier is empty
         } // end if
      } // end while

   UrlObj obj;
   obj.url = APESEARCH::string( start, frontQPtr );
   obj.priority = 69;

   // Increment pointer now that url has been copied
   if ( ++frontQPtr == frontQEnd )
      {
      popNewBatch();
      }
   return obj;
   }

void SetOfUrls::enqueue( const APESEARCH::string &url )
   {
   
   assert( numOfUrlsInserted < SetOfUrls::maxUrls );

   write( back.getFD(), url.cbegin(), url.size() );

   // Mark as a delimiter
   write( back.getFD(), "\n", 1 );
   ++numOfUrlsInserted;

   if ( numOfUrlsInserted == SetOfUrls::maxUrls )
      {
      // Used to signify end of file
      write( back.getFD(), "\r", 1 );
      startNewFile();
      numOfUrlsInserted = 0; // Reset
      } // end if
   } // end enqueue()

