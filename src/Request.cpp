#include "../include/crawler/Request.h"
#include <iostream>
#include "../libraries/AS/include/AS/Socket.h"
#include "../include/crawler/SSLSocket.h" // For SSLSocket
#include "../include/crawler/ParsedUrl.h"
#include <utility> // for std::move

#include "../libraries/AS/include/AS/algorithms.h"

#include <stdlib.h> /* atoi */


#include <iostream>

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
    APESEARCH::pair<const char *, size_t> req = url.getReqStr();
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
         char *endOfHeader =  endHeaderPtr = getHeader( socket );
         if ( !endOfHeader ) // If end of file is reached w.o. coming across header sentinel
            return Result( getReqStatus::badHtml );
         resetState(); // set all bites to zero
         res = parseHeader(endOfHeader);

         getBody();

         break;
         // Parse header
         }
      //TODO Add error catching functionality for errono in Socket and SSLSocket
      catch(...)
         {
         address.info = address.info->ai_next ? address.info->ai_next : address.info = address.head;
         ++attempts;
         }
      } // end while

   return res;
    
}  // end parseRequest()  

char * findString(char * begin, const char* end , const char *str )
   {
   const char *place = str;
   while(*place && begin != end)
      (*place == *begin++) ? ++place : place = str;
   return begin;
   } // end findString

/*
 * REQUIRED: strLookingFor to be a c-string
*/
static char const *safeStrNCmp( char const * start, const char * const end, const char* strLookingFor )
   {
   for (; *strLookingFor && start != end; ++start, ++strLookingFor )
      {
      if ( *strLookingFor != *start )
         return end;
      } // end while
   return start;
   } // end

static bool strCmp( char const *start, const char * const end, const char* strLookingFor )
   {
   for (; *strLookingFor && start != end; ++start, ++strLookingFor )
      {
      if ( *strLookingFor != *start )
         return end;
      } // end while
   return !*strLookingFor;
   }
// HTTP/1.x 200 OK\r\n
// HTTP/2.0 200
int Request::evalulateRespStatus( char **header, const char* const endOfHeader )
   {
   static constexpr char * const newline = "\r\n";
   char *endOfLine;
   int status = -1;
   if ( (endOfLine = findString( *header, endOfHeader, newline ) ) - *header > 2 )
      {
      // Seek the space i.e HTTP/1.x" " 
      char *space = findString( *header, endOfLine, " " ); // Go one past space
      // Check if http verision is okay
      if ( space == endOfLine || findString( *header, space, "HTTP/1." ) == space )
         return -1;

      *header = space; // update pointer

      // Now seek for next space in order to add a null character there
      space = APESEARCH::find( *header, endOfLine, ' ' );
      if ( space == endOfLine )
         return -1; 
      *space = '\0'; // Change to null-character
      status = atoi( *header );

      std::cout << "Response: " << status << std::endl;
      } // end if
   *header = endOfLine; // Skip to the end of the line
   return status;
   } // end getResponseStatus()

Result Request::getResponseStatus( char **header, const char* const endOfHeader )
   {
   int status = evalulateRespStatus( header, endOfHeader );
   if ( status < 0 )
      return Result( getReqStatus::badHtml );
   unsigned unsignedStatus = static_cast<unsigned> ( status );
   return Result( validateStatus( unsignedStatus ), unsignedStatus );
   } // end evalulateRespStatus()


getReqStatus Request::validateStatus( unsigned status )
   {
   int category = status / 100;

   switch( category )
      {
      case successful:
         return getReqStatus::successful;
      case redirection:
         return getReqStatus::redirected;
      case serverError:
         return getReqStatus::ServerIssue;
      default:
         return getReqStatus::badHtml;
      } // end switch
   } // end validateStatus()


template<class Predicate, class fieldTerminator>
void processField( char const * headerPtr, char const * const endOfLine, const char* const key, Predicate valueProcess, fieldTerminator fieldTerm)
   {
   headerPtr = safeStrNCmp( headerPtr, (char *)endOfLine, key );
   if ( headerPtr == endOfLine )
      return;

   // Found the key, now processing value
   const char *endVal;
   while ( ( endVal = APESEARCH::findChars( headerPtr, endOfLine, fieldTerm ) ) != endOfLine )
      {
      valueProcess( headerPtr, endVal++ );
      headerPtr = endVal;
      } // end while
   } // end processField

Result Request::parseHeader( char const * const endOfHeader )
{
   static constexpr char * const newline = "\r\n";
   char *headerPtr = &*buffer.begin(); // Pointer to where request is in header
   char *endOfLine; // Relatie pointer to end of a specific line for a header field
   
   // Assume connection HTTP/1.x 200 OK\r\n
   Result resultOfReq( getResponseStatus( &headerPtr, endOfHeader ) );
   if ( static_cast<int>( resultOfReq.response ) < 2 )
      return resultOfReq;
   else if ( resultOfReq.status == getReqStatus::redirected )
      {
      redirect = true;
      }

   // Evaluate each header's fields
   while ( ( endOfLine = findString( headerPtr, endOfHeader, newline ) ) != endOfHeader )
      {
      switch( *headerPtr )
         {
         case 'T':
            {
            auto Tpred = [this]( char const * front, char const *end ) 
               {  
               if ( strCmp( front, end, "chunked" ) )
                  chunked = foundChunked = true;
               else if ( strCmp( front, end, "gzip" ) )
                  gzipped = foundGzipped = true; 
               }; // end pred
            if ( !foundChunked || !foundGzipped )
               processField( headerPtr, endOfLine, "Transfer-Encoding: ", Tpred, FieldTerminator() );
            break;
            } // end case 'T'
         case 'C':
            {
            auto Cpred = [this]( char const * front, char const * end ) 
               {  
               if ( strCmp( front, end, "gzip" ) )
                  gzipped = foundGzipped = true;
               }; // end pred
            if ( !foundGzipped )
               processField( headerPtr, endOfLine, "Content-Encoding: ", Cpred, FieldTerminator() );
            break;
            } // end case 'C'
         case 'L':
            {
            auto Lpred = [&resultOfReq, this]( char const * front, char const *end ) 
               {  
               resultOfReq.url = std::string( front, end );
               foundUrl = true;
               }; // end pred
            if ( redirect && !foundUrl )
               processField( headerPtr, endOfLine, "Location: ", Lpred , []( char c) { return c == '\r'; } );
            break;
            } // end case 'L'
         } // end switch
      headerPtr = endOfLine;
      } // end while
   return resultOfReq;
}

void Request::getBody()
   {
   return;
   }

APESEARCH::pair< std::string, size_t> Request::getResponseBuffer()
   {
   return APESEARCH::pair< std::string, size_t> ( std::move( buffer ), 0);
   }

