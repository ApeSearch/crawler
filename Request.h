
#pragma once

#ifndef REQUEST_H_APESEARCH
#define REQUEST_H_APESEARCH

#define MAXATTEMPTS 5


#include <memory>
#include <string>
#include "Address.h"
#include "ParsedUrl.h"
#include "utility.h"
#include "unique_ptr.h"
#include "Socket.h"
enum class getReqStatus
{
   successful, // 
   redirected, // What url to redirected ( give to frontier for them to check )
   timedOut,   // 
   badHtml     // Tell crawler to throw away such html
};

// Tells the crawler status of request
struct Result
{
   std::string url; // For redirects...
   getReqStatus status; 
   Result() = default;
   Result( getReqStatus _status ) : status( _status ) {}
};

#define TIMEOUT 30
class Request 
{


   std::string buffer;
   static constexpr const char * const fields = "User-Agent: ApeSearchCrawler/1.0 xiongrob@umich.edu (Linux)\r\n\
   Accept: */*\r\n Accept-Encoding: identity\r\nConnection: close\r\n\r\n";
   static constexpr const size_t fieldSize = 139u;
   static constexpr time_t timeoutSec = 30;
   char *endOfHeader;
   unsigned state;
   bool gzipped, chunked, redirect;
   // The first arg holds the buffer, second argument tells which index response starts (in case includes header)
   APESEARCH::pair< std::string, size_t> processedResponse; 
   

public:
   //TODO Overload string to not care about what's inside
   Request();
   char *getHeader( APESEARCH::unique_ptr<Socket> &socket );

   Result getReqAndParse( const char* );

   Result parseHeader(const char*);

   void getBody();

   APESEARCH::pair< std::string, size_t> getResponseBuffer();

};

#endif