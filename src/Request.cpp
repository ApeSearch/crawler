#include "../include/crawler/Request.h"
#include <iostream>
#include "../libraries/AS/include/AS/utility.h" // for APESEARCH::pair
#include "../libraries/AS/include/AS/Socket.h"
#include "../include/crawler/SSLSocket.h" // For SSLSocket
#include "../include/crawler/ParsedUrl.h"
#include <utility> // for std::move
using APESEARCH::pair;

#ifdef testing
   #include <memory>
   using std::unique_ptr;
   #include <string>
   using std::string;
#else
   #include "../libraries/AS/include/AS/unique_ptr.h"
   using APESEARCH::unique_ptr;
#endif


Request::Request() : buffer(65536, '\0'){}

char *Request::getHeader( unique_ptr<Socket> &socket )
    {
    int bytesToWrite = 0;
    static char const * const endHeader = "\r\n\r\n";
    char const *place = endHeader;
    
    char *bufPtr, *bufEnd;
    bufEnd = bufPtr = &*buffer.begin();
    //TODO check string.end() 

    while( *place && (bytesToWrite = socket->receive(bufPtr, static_cast<int> ( &*buffer.end() - bufPtr) ) ) > 0 )
        {
        bufEnd += bytesToWrite;
        while(*place && bufPtr != bufEnd)
        (*place == *bufPtr++) ? ++place : place = endHeader;
        //TODO Some statistics tracking to see how many times this runs
        if ( bufPtr == &*buffer.end() )
            {
            unsigned sizeOfHeaderSoFar = static_cast<unsigned> ( buffer.size() );
            //TODO implemnent resize for string.h
            buffer.resize( buffer.size() << 1 );
            bufPtr = bufEnd = &*buffer.begin() + sizeOfHeaderSoFar;
            }
        }
        //construct string based off of buffer call our pase header function
        // Reached the end of header
        
        std::cout << std::string(&*buffer.begin(), bufPtr) << std::endl;
        return *place ? nullptr : bufPtr;
    } 


Result Request::getReqAndParse(const char *urlStr)
{
    ParsedUrl url( urlStr );
    Address address(url.Host, "80");
    pair<const char *, size_t> req = url.getReqStr();
    bool httpProtocol = strcmp(url.Service, "http://"); // 0 if http 1 if https
    int attempts = 0;
    Result res;
    while(attempts < MAXATTEMPTS)
      {
      try
         {
         unique_ptr<Socket> socket( httpProtocol ? new Socket(address, Request::timeoutSec) : new SSLSocket(address, timeoutSec) );
         socket->send( req.first(), static_cast<int>( req.second() ) ); // Send get part
         socket->send( fields, fieldSize ); // Send fields
         char *endOfHeader = getHeader( socket );
         if ( !endOfHeader ) // If end of file is reached
            return Result( getReqStatus::badHtml );
         res = parseHeader(endOfHeader);

         getBody();
         
         // Parse header
         }
      //TODO Add error catching functionality for errono in Socket and SSLSocket
      catch(...)
         {
         address.info = address.info->ai_next ? address.info->ai_next : address.info = address.head;
         ++attempts;
         }
      } // end while

   return Result();
    
}  // end parseRequest()  

char * findString(char * begin, const char* end , const char *str )
   {
   const char *place = str;
   while(*str && begin != end)
      (*place == *begin++) ? ++place : place = str;
   return begin;
   } // end findString

char * findString( char *begin, const char* end, const char *str, const char sentinel )
   {
   const char *place = str;
   while(*str && begin != end && *begin != sentinel )
      (*place == *begin++) ? ++place : place = str;
   return begin;
   } // end findString


// HTTP/1.x 200 OK\r\n
// HTTP/2.0 200
unsigned getResponseStatus( char **header, const char* const endOfHeader )
   {
   static constexpr char * newline = "\r\n";
   char *endOfLine;
   if ( (endOfLine = findString( *header, endOfHeader, newline ) ) - *header > 2 )
      {
      char *space = findString( *header, endOfLine, " " );
      //if ( space != endOfLine )

      } // end if
   return 0;
   }

char *safeStrNCmp( char *start, char *end, const char* const strLookingFor, size_t numOfContents )
   {
   const char *place = strLookingFor;
   while ( *place && start != end )
      {
      if ( *place != *start )
         return end;
      } // end for
   //return *place ? start;
   return nullptr;
   } // end 


//Transfer-Encoding: gzip

Result Request::parseHeader( char const * const endOfHeader)
{

   const char * newline = "\r\n";
   char *headerPtr = &*buffer.begin(); // Pointer to where request is in header
   char *endOfLine; // Relatie pointer to end of a specific line for a header field
   
   // Assume connection HTTP/1.x 200 OK\r\n
   unsigned status = getResponseStatus( &headerPtr, endOfHeader );
   if ( status == -1 )
      return Result( getReqStatus::badHtml );

   while ( ( endOfLine = findString( headerPtr, endOfHeader, newline ) ) - headerPtr > 2 )
      {
      switch( *headerPtr )
      {
      case 'T':
         headerPtr = findString( headerPtr, endOfLine, "Transfer-Encoding: " );

         break;
      case 'C':

         break;
      case 'L':
         break;
      } // end switch

      
      }  //end while
   
   //scanf(ptr, "%s:%s/r/n", key, val);
   return Result();
}



void Request::getBody()
{
   
}

APESEARCH::pair< std::string, size_t> Request::getResponseBuffer()
   {
   return APESEARCH::pair< std::string, size_t> ( std::move( buffer ), 0);
   }

