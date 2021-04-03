
#pragma once

#ifndef REQUEST_H_APESEARCH
#define REQUEST_H_APESEARCH

#define MAXATTEMPTS 5


#include <memory>
#include <string>
#include "../../libraries/AS/include/AS/Address.h"
#include "ParsedUrl.h"
#include "../../libraries/AS/include/AS/utility.h"
#include "../../libraries/AS/include/AS/unique_ptr.h"
#include "../../libraries/AS/include/AS/Socket.h"
enum class getReqStatus
{
   successful, // 
   redirected, // What url to redirected ( give to frontier for them to check )
   timedOut,   // 
   badHtml,     // Tell crawler to throw away such html
   ServerIssue
};

// Tells the crawler status of request
struct Result
{
   std::string url; // For redirects...
   getReqStatus status; 
   unsigned response;
   Result() = default;
   Result( getReqStatus _status, unsigned _response = 600 ) : status( _status ), response( _response ) {}
};

#define TIMEOUT 30
class Request 
{
   enum statusCategory
   {
      informational = 1,
      successful = 2,
      redirection = 3,
      clientError = 4,
      serverError = 5
   };

   //e.g. Accept: text/plain, text/html
   struct FieldTerminator {
         FieldTerminator() = default;
         inline bool operator()(char a) {
            switch(a) 
            {
            case '\n':
            case ' ':
            case ',':
               return true;
            default:
               return false;
            } // end switch
         }  // end operator()()
   };

   std::string buffer;
   char *endHeaderPtr;
   unsigned state;
   bool gzipped, chunked, redirect;
   bool foundGzipped, foundChunked, foundUrl;
   // The first arg holds the buffer, second argument tells which index response starts (in case includes header)
   APESEARCH::pair< std::string, size_t > processedResponse;
   
   // Helper Functions
  
   // Static Variables
   static constexpr const char * const fields = "User-Agent: ApeSearchCrawler/1.0 xiongrob@umich.edu (Linux)\r\n\
   Accept: */*\r\n Accept-Encoding: identity\r\nConnection: close\r\n\r\n";
   static constexpr const size_t fieldSize = 139u;
   static constexpr time_t timeoutSec = 30;

   inline void resetState()
      {
      gzipped = chunked = redirect = foundGzipped = foundChunked = foundUrl = false; // Reset state
      }

public:
   getReqStatus validateStatus( unsigned status );
   int evalulateRespStatus( char **header, const char* const endOfHeader );
   Result getResponseStatus( char **header, const char* const endOfHeader );

   Request();

   char *getHeader( APESEARCH::unique_ptr<Socket> &socket );

   Result getReqAndParse( const char* );

   Result parseHeader(const char*);

   void getBody();

   APESEARCH::pair< std::string, size_t> getResponseBuffer();

};

#endif