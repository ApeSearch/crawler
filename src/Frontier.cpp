
#include "../include/crawler/Frontier.h"

std::size_t UrlFrontier::FrontEndPrioritizer::pickQueue()
   {
    return 1;
   }

UrlObj UrlFrontier::FrontEndPrioritizer::getUrl( SetOfUrls& set )
   {
    return set.dequeue();
   }

void UrlFrontier::FrontEndPrioritizer::putUrl()
   {
   return;
   }

UrlObj UrlFrontier::BackendPolitenessPolicy::obtainRandUrl()
   {
   
   return UrlObj();
   }

UrlObj UrlFrontier::BackendPolitenessPolicy::getMostOkayUrl()
   {
    
    return UrlObj();
   }

unsigned UrlFrontier::ratingOfTopLevelDomain( const char * )
   {
    return 1;
   }

UrlFrontier::UrlFrontier( const char *directory ) : set( directory )
   {
   }

APESEARCH::string UrlFrontier::getNextUrl( )
   {
    UrlObj obj( backEnd.getMostOkayUrl() );
    return obj.url;
   }
bool UrlFrontier::insertNewUrl( APESEARCH::string&& url )
   {
    return true;
   }

bool UrlFrontier::BackendPolitenessPolicy::insertTiming( domainTiming&& timing  )
   {
   std::string str( timing.domain.begin(), timing.domain.end() );
   std::unordered_map<std::string, size_t>::iterator itr;
{
   APESEARCH::unique_lock<APESEARCH::mutex> uniqMapLk( mapLk );
   itr = backendDomains.find( str );
} // ~uniqMapLk
   if ( itr != backendDomains.end() )
      {
      APESEARCH::unique_lock<APESEARCH::mutex> uniqPQLk( pqLk );
      backendHeap.emplace( std::forward<domainTiming>( timing ) );
      return true;
      } // end if
   return false;
   }