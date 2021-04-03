
#include "../include/crawler/Frontier.h"
#include "../libaries/AS/include/AS/unique_mmap.h"

UrlFrontier::UrlFrontier( ) : frontEnd( ), backEnd( ), set()
   {
   }

UrlFrontier::UrlFrontier( const char *file ) : frontEnd( ), backEnd( ), set( file )
   {
   }

void UrlFrontier::run()
   {
   
   }