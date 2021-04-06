
#include "../include/crawler/Frontier.h"
#include "../libaries/AS/include/AS/unique_mmap.h"



SetOfUrls::SetOfUrls() : SetOfUrls( SetOfUrls::frontierLoc )
   {
   }

SetOfUrls::SetOfUrls( const char *file ) : fin( file )
   {
   if ( !fin.open() )
      {
      std::runtime_error( "VirtualFileSytem couldn't be opened" );
      } // end if
   }

UrlObj SetOfUrls::dequeue()
   {
   UrlObj obj;
   sd::string temp;
   std::getline( fin, temp );

   // Remove from file
   temp.replace( temp.find( temp ), temp.length(), "" );

   obj.url = APESEARCH::string( temp.begin(), temp.end() );

   obj.priority = 69;

   return obj;
   }





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