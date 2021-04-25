
#pragma once

#ifndef REQUEST_H_APESEARCH
#define REQUEST_H_APESEARCH

#define MAXATTEMPTS 3


#include <memory>
#include <string>
#include "../../libraries/AS/include/AS/Address.h"
#include "ParsedUrl.h"
#include "../../libraries/AS/include/AS/utility.h"
#include "../../libraries/AS/include/AS/unique_ptr.h"
#include "../../libraries/AS/include/AS/Socket.h"
#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/string.h"

#include <math.h>       /* pow */
enum class getReqStatus
{
   successful, // 
   redirected, // What url to redirected ( give to frontier for them to check )
   timedOut,   // 
   badHtml,     // Tell crawler to throw away such html
   notHtml,
   badURL,
   ServerIssue
};

// Tells the crawler status of request
struct Result
{
   APESEARCH::string url; // For redirects...
   getReqStatus status; 
   unsigned response;
   Result() = default;
   Result( getReqStatus _status, unsigned _response = 600 ) : status( _status ), response( _response ) {}
};

class Request 
{
#ifdef DEBUG
   public:
#endif
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
            case ';':
               return true;
            default:
               return false;
            } // end switch
         }  // end operator()()
   };
   static constexpr size_t maxBodyBytes = 33554432; //2**25
   APESEARCH::vector< char > headerBuff;
   APESEARCH::vector< char > bodyBuff;
   //std::string buffer;
   std::size_t contentLengthBytes = 0;
   unsigned state;
   bool gzipped, chunked, redirect, contentLength, headerBad, isHtml;
   bool foundGzipped, foundChunked, foundUrl, foundContentLength;
   // The first arg holds the buffer, second argument tells which index response starts (in case includes header)
   APESEARCH::pair< std::string, size_t > processedResponse;
   
   // Helper Functions
  
   // Static Variables
   static constexpr const char * const fields = "User-Agent: ApeSearch Crawler/2.0 apesearchnoreply@gmail.com (Linux)\r\nAccept: */*\r\nAccept-Encoding: gzip\r\nConnection: close\r\n\r\n";
   //static constexpr const char * const fields = "User-Agent: curl/7.64.1\r\nAccept: */*\r\n\r\n";
   static constexpr const size_t fieldSize = 139u;
   static constexpr time_t timeoutSec = 7;

   inline void resetState()
      {
      gzipped = chunked = redirect = foundGzipped = foundChunked = foundUrl = contentLength = foundContentLength = headerBad = false; // Reset state
      contentLengthBytes = 0;
      if ( !bodyBuff.empty( ) )
         bodyBuff = APESEARCH::vector< char >( );
      }
   
   void receiveNormally( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody );
   void chunkedHtml( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody);

public:
   getReqStatus validateStatus( unsigned status );
   int evalulateRespStatus( char **header, const char* const endOfHeader );
   Result getResponseStatus( char **header, const char* const endOfHeader );

   Request();

   APESEARCH::pair< char const * const, char const * const > getHeader( APESEARCH::unique_ptr<Socket> &socket );

   Result getReqAndParse( const char* );

   Result parseHeader(const char*);

   void getBody( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody );

   APESEARCH::vector< char > getResponseBuffer();

};

#endif
