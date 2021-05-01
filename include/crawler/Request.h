
#pragma once
#ifndef REQUEST_H_APESEARCH
#define REQUEST_H_APESEARCH

// The amount of times a request object will iterate
#define MAXATTEMPTS 3

#include <memory> // Required for std::runtime_error
#include "ParsedUrl.h" // Parsing and extracting different sections of a url
#include "../../libraries/AS/include/AS/Address.h" // For Address (RAII that frees address after the object goes out of scope)
#include "../../libraries/AS/include/AS/utility.h" // For APESEARCH::pair
#include "../../libraries/AS/include/AS/unique_ptr.h" // mainly for dynamic dispatch with sockets and sslsockets ( socket -> sslsocket )
#include "../../libraries/AS/include/AS/Socket.h" // Required for declaractions
#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/string.h"


enum class getReqStatus
{
   successful, // The request was successful and the html was downloaded without issues
   redirected, // What url to redirected ( give to frontier for them to check for the bloomfilter (etc) )
   timedOut,   // The request timed out
   badHtml,    // Tell crawler to throw away such html
   notHtml,    // Tells crawler this is not html ( from content-type )
   badURL,     // Tells crawler that couldn't get a response
   ServerIssue // 50x issue (maybe try again?)
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

/*
 * A tail-recursive function for calculating interger powers at compile time. 
*/
constexpr int64_t ipow( int64_t base, int exp, int64_t result = 1 )
   {
   return exp < 1 ? result : ipow( base * base, exp / 2, ( exp & 1 ) ? result * base : result );
   }

class Request 
{
#ifdef DEBUG
   public:
#endif
   // U
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
   static constexpr size_t maxBodyBytes = ipow( 2, 25 ); //2**25
   APESEARCH::vector< char > headerBuff;
   APESEARCH::vector< char > bodyBuff;
   std::size_t contentLengthBytes = 0;
   char const *urlPtr;
   unsigned state;
   bool gzipped, chunked, redirect, contentLength, headerBad, isHtml;
   bool foundGzipped, foundChunked, foundUrl, foundContentLength;

   // Helper Functions

   // Static Variables
   static constexpr const char * const fields = "User-Agent: ApeSearch Crawler/2.0 apesearchnoreply@gmail.com (Linux)\r\nAccept: */*\r\nAccept-Encoding: gzip\r\nConnection: close\r\n\r\n";
   static constexpr const size_t fieldSize = 139u;
   static constexpr time_t timeoutSec = 7;

   inline void resetState()
      {
      gzipped = chunked = redirect = foundGzipped = foundChunked = foundUrl = contentLength = foundContentLength = headerBad = isHtml = false; // Reset state
      contentLengthBytes = 0;
      if ( !bodyBuff.empty( ) )
         bodyBuff = APESEARCH::vector< char >( );
      } // end resetState( )
   
   void receiveNormally( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody );
   void chunkedHtml( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody);

   // Helper functions for chunkedHtml
   ssize_t findChunkSize( APESEARCH::unique_ptr<Socket> &socket, char **ptr, char const **currEnd, APESEARCH::vector<char>& buffer );
   bool writeChunked( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::vector<char>& buffer, char **ptr, char const **currEnd, const size_t bytesToReceive );
   bool attemptPushBack( char val );
public:
   Request();

   // Functions related to response status
   getReqStatus validateStatus( unsigned status );
   int evalulateRespStatus( char **header, const char* const endOfHeader );
   Result getResponseStatus( char **header, const char* const endOfHeader );


   APESEARCH::pair< char const * const, char const * const > getHeader( APESEARCH::unique_ptr<Socket> &socket );

   Result getReqAndParse( const char* );

   Result parseHeader(const char*);

   void getBody( APESEARCH::unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody );

   APESEARCH::vector< char > getResponseBuffer();
};

#endif
