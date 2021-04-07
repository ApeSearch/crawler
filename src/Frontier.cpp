
#include "../include/crawler/Frontier.h"
#include "../libaries/AS/include/AS/unique_mmap.h"
#include "../libraries/AS/include/AS/File.h"
#include "../libraries/AS/include/AS/algorithms.h" // for APESEARCH::copy

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
static struct Dirent *getNextDirEntry( DIR *dir )
   {
   Dirent *dp;
   while( (dp = readdir (dir)) != NULL && 
      ( dp->d_type != DT_REG || !strcmp(dp->d_name, ".") || strcmp(dp->d_name, "..") ) );
   return dp;
   }


SetOfUrls::SetOfUrls() : SetOfUrls( SetOfUrls::frontierLoc )
   {
   }


SetOfUrls::SetOfUrls( const char *directory ) : cwd( PATH_MAX ), numOfUrlsInserted( 0 )
   {
   getcwd( cwd, PATH_MAX );
   shrinkSize( cwd, strlen( cwd.front() ) );

   if ( dir = opendir( directory ) == NULL )
      {
      perror( directory );
      std::runtime_error( "VirtualFileSytem couldn't be opened" );
      } // end if
   // Skip until a valid file has been reached
   
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
   // Open a new temp file
   back = File( cwd, O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR );

   // Memory map it...
   backOfQueue = APESEARCH::unique_mmap temp( 0, bytesReq, PROT_READ | PROT_WRITE, MAP_PRIVATE, back.getFD(), 0 );
   backQPtr = reinterpret_cast< char *>( temp.get() );
   return temp;
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
   if ( ( dp = getNextDirEntry( dir ) ) )
      {
      File file( dp->d_name, O_RDONLY );
      int fd = file.getFD();
      frontOfQueue = APESEARCH::unique_mmap( 0, FileSize( fd ), PR  );
      frontQPtr = reinterpret_cast< char *>( frontOfQueue.get() );
      } // end if
   else
      frontQPtr = nullptr;
   return dp;
   } // end popNewBack()

void SetOfUrls::finalizeSection( )
   {
   static unsigned id = 0;

   cwd.push_back( id++ ); // Add a unique identifier
   cwd.push_back( 0 ); // Add null-character
   // Create a hard link
   linkat( back.getFD(), NULL, AT_FDCWD, &cwd.front(), AT_EMPTY_PATH ); 

   // Modifies
   startNewFile();
   }


UrlObj SetOfUrls::dequeue()
   {
   if ( !frontQPtr && !popNewBatch() )
      {
       return UrlObj(); // Frontier is empty
      }

   char character;
   char *start = frontQPtr;
   for ( ;( character = *frontQPtr ) != '\n'; ++frontQPtr )
      {
      // Finished with file and now has to go to new one...
      if ( character == '\r' )
         {
         if ( !popNewBatch() )
            start = frontQPtr;
         else 
            return UrlObj(); // Frontier is empty
         } // end if
      } // end for

   UrlObj obj;
   obj.url = APESEARCH::string( character, frontQPtr );
   obj.priority = 69;

   // Increment pointer now that url has been copied
   ++frontQPtr;

   return obj;
   }

void enqueue( const APESEARCH::string &url )
   {
   
   assert( numOfUrlsInserted < SetOfUrls::maxUrls );

   backQPtr = APESEARCH::copy( url.front(), url.end(), backQPtr );

   // Mark as a delimiter
   *backQPtr++ = '\n';
   ++numOfUrlsInserted;

   if ( numOfUrlsInserted == SetOfUrls::maxUrls )
      {
      // Used to signify end of file
      *backQPtr = '\r';
      startNewFile();
      numOfUrlsInserted = 0; // Reset
      } // end if
   } // end enqueue()


UrlFrontier::UrlFrontier( ) : frontEnd( ), backEnd( ), set()
   {
   // Start threads that add to queue...
   }

UrlFrontier::UrlFrontier( const char *file ) : frontEnd( ), backEnd( ), set( file )
   {
   }

void UrlFrontier::run()
   {
   
   }