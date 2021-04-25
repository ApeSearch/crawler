#include "../include/crawler/Request.h"
#include <iostream>
#include "../libraries/AS/include/AS/Socket.h"
#include "../include/crawler/SSLSocket.h" // For SSLSocket
#include "../include/crawler/ParsedUrl.h"
#include <utility> // for std::move

#include "../libraries/AS/include/AS/algorithms.h" // use Algorithms

#include <stdlib.h> /* atoi */
#include <zlib.h>
#include <array>
#include <vector>
#include <string.h>

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


Request::Request() : headerBuff( 16384, 0 ) {}

APESEARCH::pair< char const * const, char const * const > Request::getHeader( unique_ptr<Socket> &socket )
    {
    int bytesReceived = 0;
    static char const * const endHeader = "\r\n\r\n";
    char const *place = endHeader;
    
    char *bufPtr, *headerEnd;
    headerEnd = bufPtr = &*headerBuff.begin();
    //TODO check string.end() 

    while( *place && static_cast<size_t > ( headerEnd - headerBuff.begin( ) ) < headerBuff.size( ) && ( bytesReceived = socket->receive( bufPtr, static_cast<size_t> ( &*headerBuff.end() - bufPtr ) ) ) > 0 )
        {
        headerEnd += bytesReceived;
        while( *place && bufPtr != headerEnd )
            (*place == *bufPtr++) ? ++place : place = endHeader;
        //TODO Some statistics tracking to see how many times this runs

        } // end while

   //construct string based off of buffer call our pase header function
   // Reached the end of header

   //std::cout << std::string(  &headerBuff.front(), bufPtr ) << std::endl;

   return APESEARCH::pair< char const * const, char const * const  > 
      ( ( *place ? nullptr : bufPtr ), ( *place ? nullptr : headerEnd ) );
    } 


/*
 * Where the request is done
*/
Result Request::getReqAndParse(const char *urlStr)
{
   urlPtr= urlStr;
    ParsedUrl url( urlStr );
    APESEARCH::pair<const char *, size_t> req = url.getReqStr();
    bool httpProtocol = !strcmp(url.Service, "http"); // 0 if http 1 if https
    Address address(url.Host, httpProtocol ? "80" : "443" );
    if(!address.valid)
      return Result( getReqStatus::badURL );
    int attempts = 0; 
    Result res;
    while(attempts < MAXATTEMPTS)
      {
      try
         {
         unique_ptr<Socket> socket( httpProtocol ? new Socket(address, Request::timeoutSec) : new SSLSocket(address, timeoutSec) );
         char buff[1024];
         snprintf( buff, sizeof( buff ), "%s%s", req.first( ), fields );
         //printf( "%s\n", buff );
         socket->send( buff, strlen( buff ) );
         APESEARCH::pair< char const * const, char const * const  > headerPtrs( getHeader( socket ) );
         // first is end of header, second is end of buffer header is situated
         assert( headerPtrs.first( ) <= headerPtrs.second( ) );

         if ( !headerPtrs.first( ) ) // If end of file is reached w.o. coming across header sentinel
            return Result( getReqStatus::badHtml );
         resetState(); // set all bits to zero

         // Parse header
         res = parseHeader( headerPtrs.first( ) );

         if ( headerBad || res.status == getReqStatus::badHtml || !( foundChunked ^ foundContentLength ) 
            || foundContentLength && (contentLengthBytes > Request::maxBodyBytes || 
               headerPtrs.second( ) - headerPtrs.first( ) > ( ssize_t ) contentLengthBytes ) )
            {
            res.status = getReqStatus::badHtml;
            return res;
            } // end if
         else if ( res.status == getReqStatus::redirected )
            {
            return res;
            }
         else if ( !isHtml )
            return Result( getReqStatus::notHtml );

         getBody( socket, headerPtrs );
         if ( headerBad )
            return Result( getReqStatus::badHtml );

         return res;
         }
      //TODO Add error catching functionality for errono in Socket and SSLSocket
      catch(...)
         {
         if(errno == EWOULDBLOCK)
            return Result( getReqStatus::timedOut );
         //TODO check errno         
         address.info = address.info->ai_next ? address.info->ai_next : address.info = address.head;
         ++attempts;
         }
      } // end while

   return Result( getReqStatus::ServerIssue );
}  // end parseRequest()  

bool insenstiveCharCompare( char const *A, char const *B )
   {
   int letterA, letterB;
   letterA = *((unsigned char *)A);
   letterB = *((unsigned char *)B);
   letterA = tolower(toupper(letterA));
   letterB = tolower(toupper(letterB));
   return letterA == letterB;
   }

char * findString(char * begin, const char* end , const char *str )
   {
   const char *place = str;
   while(*place && begin != end)
      ( insenstiveCharCompare( begin++, place ) )  ? ++place : place = str;

   return begin;
   } // end findString

/*
 * REQUIRED: strLookingFor to be a c-string
*/
static char const *safeStrNCmp( char const * start, const char * const end, const char* strLookingFor )
   {
   for (; *strLookingFor && start != end; ++start, ++strLookingFor )
      {
      if ( !insenstiveCharCompare( strLookingFor, start ) )
         return end;
      } // end while
   return start;
   } // end

static bool strCmp( char const *start, const char * const end, const char* strLookingFor )
   {
   for (; *strLookingFor && start != end; ++start, ++strLookingFor )
      {
      if ( !insenstiveCharCompare( strLookingFor, start ) )
         return false;
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
   if ( (endOfLine = findString( *header, endOfHeader, newline ) ) - *header > 2 ) // if <= 2, implies \r\n
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

      } // end if
   *header = endOfLine; // Skip to the end of the line
   return status;
   } // end getResponseStatus()

// Looks at the first line and find out what kind of file it is
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
   // Verify that key is indeed the key
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
   char *headerPtr = &*headerBuff.begin(); // Pointer to where request is in header
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
   while ( !headerBad && ( endOfLine = findString( headerPtr, endOfHeader, newline ) ) != endOfHeader )
      {
      switch( *headerPtr )
         {
         case 'T':
         case 't':
            {
            auto Tpred = [this]( char const * front, char const *end ) 
               {  
               if ( strCmp( front, end, "chunked" ) )
                  chunked = foundChunked = true;
               else if ( strCmp( front, end, "gzip" ) )
                  gzipped = foundGzipped = true; 
               }; // end pred
            if ( ( !foundChunked && !foundContentLength ) || !foundGzipped )
               processField( headerPtr, endOfLine, "Transfer-Encoding: ", Tpred, FieldTerminator() );
            break;
            } // end case 'T'
         case 'C':
         case 'c':
            {
            auto Cpred = [this]( char const * front, char const * end ) 
               {  
               if ( strCmp( front, end, "gzip" ) )
                  gzipped = foundGzipped = true;
               }; // end pred
            
            auto ContentLen = [ this ]( char const * front, char const * end )
               {
               char const *trueEnd = end - 1;

               while( front != trueEnd )
                  {
                  contentLengthBytes *= 10;
                  int digit = *front - '0';
                  if ( digit < 0 || digit > 10 )
                     {
                     contentLengthBytes = 0;
                     headerBad = true;
                     return;
                     } // end if
                  contentLengthBytes += std::size_t ( *front++ - '0' );
                  } // end while
               foundContentLength = contentLength = true; 
               };
            
            auto ContentType = [ this ]( char const * front, char const * end )  
               {
               if ( strCmp( front, end, "text/html" ) )
                  isHtml = true;
               };
            
            if ( !foundGzipped )
               processField( headerPtr, endOfLine, "Content-Encoding: ", Cpred, FieldTerminator() );
            if ( !foundChunked && !foundContentLength )
               {
               processField( headerPtr, endOfLine, "Content-Length: ", ContentLen, FieldTerminator() );
               if ( contentLengthBytes > Request::maxBodyBytes )
                  {
                  resultOfReq.status = getReqStatus::badHtml;
                  return resultOfReq;
                  } // end if
               }
            if( !isHtml )
               processField( headerPtr, endOfLine, "Content-Type: ", ContentType ,FieldTerminator() );

            break;
            } // end case 'C'
         case 'L':
         case 'l':
            {
            auto Lpred = [&resultOfReq, this]( char const * front, char const *end ) 
               {  
               resultOfReq.url = APESEARCH::string( front, end );
               foundUrl = true;
               }; // end pred
            // Only look for Location when seen in first line
            if ( redirect && !foundUrl )
               processField( headerPtr, endOfLine, "Location: ", Lpred , []( char c) { return c == '\r'; } );
            break;
            } // end case 'L'
         } // end switch
      headerPtr = endOfLine;
      } // end while
   return resultOfReq;
}

// Transfer-Encoding can be chunked

void Request::receiveNormally( unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody )
   {
   if(contentLengthBytes == 0 )
      return;
   bodyBuff.resize( contentLengthBytes );
   char *bufPtr = APESEARCH::copy( partOfBody.first( ), partOfBody.second( ), &bodyBuff.front( ) );
   char *bufEnd = &bodyBuff.front( ) + contentLengthBytes;

   while( bufPtr < bufEnd )
      bufPtr += socket->receive( bufPtr, static_cast<int>( bufEnd - bufPtr ));

   }  // end normalHtml( )

//size_t ctoh()

static ssize_t hexaToDecimal( char const *begin, char const *end )
   {
   ssize_t num = 0;
   while( begin != end )
      {
      num <<= 4;
      int hexa = *begin - '0';
      char character = *begin++;
      if ( '0' <= character && character <= '9')
         hexa = character - '0';
      else if ( 'A' <= character && character <= 'F' )
         hexa = character - 'A' + 10;
      else if ( 'a' <= character && character <= 'f' )
         hexa = character - 'a' + 10;
      else
         return -1;
      num += hexa;
      } // end while
   return num;
   }

inline bool seekLineSeperator(  unique_ptr<Socket> &socket, char ***ptr, char const ***currEnd, APESEARCH::vector<char>& buffer )
   {
   static char const * const endChunkSize = "\r\n";
   char const *start = **ptr;
   char const *place = endChunkSize;
   ssize_t bytesReceived = 0;

   do
   {
      **currEnd += bytesReceived;
      while( *place && **ptr != **currEnd )
         ( * ( **ptr )++ == *place ) ? ++place : place = endChunkSize;

      // Check if we're currently at the end of buffer
      if ( **ptr == buffer.end( ) )
         {
         if ( buffer.end( ) - start == buffer.size( ) )
            return false;
            return -1;

         size_t shift = start - buffer.begin( );

         // Copy buffer to the beginning
         char *retPtr = APESEARCH::copy( start, ( const char * ) buffer.end( ), buffer.begin( )  );
         assert( retPtr - buffer.begin( ) == shift );

         // Readjust pointers
         start = buffer.begin( ); // start now begins at the beginning
         **ptr -= shift;
         **currEnd -= shift; // currEnd is shifted down start - buffer.begin( ) bytes
         } // end if
   } while ( *place && ( bytesReceived = socket->receive( **ptr, buffer.end( ) - **ptr ) ) > 0 );
   return true;
   }

ssize_t Request::findChunkSize( unique_ptr<Socket> &socket, char **ptr, char const **currEnd, APESEARCH::vector<char>& buffer )
   {
   static char const * const endChunkSize = "\r\n";
   char const *start = *ptr;
   char const *place = endChunkSize;
   ssize_t bytesReceived = 0;

   if ( !seekLineSeperator( socket, &ptr, &currEnd, buffer ) )
      return -1;

   if ( *(*ptr - 1) != '\n' || *(*ptr - 2) != '\r' )
      {
      char const *itreator = buffer.begin( );
      while( itreator != *currEnd )
         std::cout << *itreator++;
      printf("\nIssue with Url: %s\n", urlPtr);
      assert( *(*ptr - 1) == '\n' && *(*ptr - 2) == '\r' );
      } // end if

   return hexaToDecimal( start, *ptr - 2 );
   } // end findChunkSize( )

bool Request::attemptPushBack( char val )
   {
   // If about to increase, check if would go over
   if ( bodyBuff.size( ) == bodyBuff.capacity( ) && bodyBuff.size( ) << 1 > Request::maxBodyBytes )
      return false;
   bodyBuff.push_back( val );
   return true;
   } // end val

// Continously iterate through the buffer and write to bodyBUff until bytes
bool Request::writeChunked( unique_ptr<Socket> &socket, APESEARCH::vector<char>& buffer, char **ptr, char const**currEnd, const size_t bytesToReceive )
   {
   assert( bytesToReceive > 0 );
   size_t bytesToWrite = 0;
   size_t bytesWritten = 0;

   do
   {
      *currEnd += bytesToWrite;
      while( *ptr != *currEnd )
         {
         if ( !attemptPushBack( *( *ptr )++ ) )
            return false;
         if( ++bytesWritten == bytesToReceive )
            {
            return seekLineSeperator( socket, &ptr, &currEnd, buffer );
            }
         } // end while
      *ptr = buffer.begin( );
      *currEnd = ( const char * ) buffer.begin( );
   } while( ( bytesToWrite = socket->receive( *ptr, buffer.end( ) - *ptr ) ) > 0 );
   } // end writeChunked( )

void Request::chunkedHtml(unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody)
   {
   assert( partOfBody.second() - partOfBody.first() < 65536 );
   APESEARCH::vector<char> temp( 65536 ); // 2^16
   char *buffPtr = temp.begin( );
   char const *currEnd = ( const char * ) APESEARCH::copy( partOfBody.first(), partOfBody.second(), temp.begin() );
   do
   {
   ssize_t chunkSize = findChunkSize( socket, &buffPtr, &currEnd, temp );
   if ( chunkSize == -1 )
      {
      headerBad = true;
      return;
      } // end if
   // Reached the end ( no longer receiving )
   else if ( chunkSize == 0 )
      {
      return;
      } // end elseif

   // Resets if reaches the end
   if ( buffPtr == temp.end( ) )  
      buffPtr = temp.begin( );
   
   // Now attempt to continue to write into BodyBuff
   if ( !writeChunked( socket, temp, &buffPtr, &currEnd, ( size_t ) chunkSize ) )
      {
      headerBad = true;
      return;
      } // end if
   } while ( true );
   } // end chunkedHtml( )


static void DecompressResponse( APESEARCH::vector < char >& data_ );

void Request::getBody( unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody )
   {
   
   if ( chunked )
   {
      chunkedHtml( socket, partOfBody );
   }
   else
      receiveNormally( socket, partOfBody );
      
   if ( !headerBad && gzipped )
      DecompressResponse( bodyBuff );

   return;
   }

   APESEARCH::vector< char > Request::getResponseBuffer()
   {
      return std::move( bodyBuff );
   }

// Perform the decompression
void DecompressResponse( APESEARCH::vector < char >& data_ )
    {
    z_stream zs;                        // z_stream is zlib's control structure
    memset( &zs, 0, sizeof( zs ) );

    APESEARCH::vector < char > decompressed;
    std::array < char, 32768 > outBuffer;

    if ( inflateInit2( &zs, MAX_WBITS + 16 ) != Z_OK )
      throw std::runtime_error( "inflateInit2 fail" );

    zs.next_in =
        const_cast < Bytef * >( reinterpret_cast < const unsigned char * >( &data_.front( ) ) );
    zs.avail_in = static_cast < unsigned int >( data_.size( ) );

    int ret;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do
        {
        zs.next_out = reinterpret_cast < Bytef* >( outBuffer.data( ) );
        zs.avail_out = sizeof( outBuffer );

        ret = inflate( &zs, 0 );

        if ( decompressed.size( ) < zs.total_out )
            {
            unsigned bytesToTransfer = zs.total_out - decompressed.size( );
            unsigned sizeOfDec = decompressed.size( );
            decompressed.resize( decompressed.size( ) + bytesToTransfer );
            char *decompressedPtr = &decompressed.front( ) + sizeOfDec;
            APESEARCH::copy( outBuffer.data( ), outBuffer.data( ) + zs.total_out - sizeOfDec, decompressedPtr );
            } // end if
        }
    while ( ret == Z_OK );

    inflateEnd( &zs );

    if ( ret != Z_STREAM_END )
        {          // an error occurred that was not EOF
        throw std::runtime_error( "Non-EOF occurred while decompressing" );
        }

    APESEARCH::swap( decompressed, data_ );
    }

/*
void Request::chunkedHtml(unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody)
{
   APESEARCH::vector<char> temp;
   temp.resize(262144);
   APESEARCH::copy(partOfBody.first(), partOfBody.second(), temp.begin());
   int total_read = partOfBody.second() - partOfBody.first();
   

   while(true)
   {
      if(temp.size() > maxBodyBytes)
      {
         headerBad = true;
         return;
      }
      if((3*temp.size())/4 < total_read)
         temp.resize(temp.size()*2);

      int recvd = socket->receive( temp.begin() + total_read, temp.size() - total_read );
      total_read += recvd;
      //Stupid but simple
      if( total_read > 5 && temp[total_read - 1] == '\n' && temp[total_read - 2] == '\r'
      && temp[total_read - 3] == '\n' && temp[total_read - 4] == '\r' && temp[total_read - 5] == '0')  
         break;
      
   }  
   
   int start = 0;
   while(start < total_read)
   {
      
      for(int i = start; i < total_read; ++i)
      {
         //we hit the end
         if( temp[i] == '\r')
         { 
            ssize_t hex = hexaToDecimal(temp.begin() + start, temp.begin() + i) + 4;
            //newline them out for parser to handle
            start = i + hex;
            if(start < total_read)
            {
               for(int j = i + 2; j < start - 2 ;++j)
               {
                  bodyBuff.push_back(temp[j]);
               }
            }
            break;
         }
      }
   }
}
*/
