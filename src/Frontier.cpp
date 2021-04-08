
#include "../include/crawler/Frontier.h"

std::size_t UrlFrontier::FrontEndPrioritizer::pickQueue()
   {
    return 1;
   }

UrlObj UrlFrontier::FrontEndPrioritizer::getUrl()
   {
    return UrlObj();
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

void UrlFrontier::getNextUrl( APESEARCH::string& buffer )
   {
    return;
   }
bool UrlFrontier::insertNewUrl( APESEARCH::string&& url )
   {
    return true;
   }