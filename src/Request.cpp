#include <iostream>
#include <utility> // for std::move
#include "../include/crawler/Request.h"
#include "../libraries/AS/include/AS/Socket.h" // For Regular Socket
#include "../include/crawler/SSLSocket.h" // For SSLSocket
#include "../include/crawler/ParsedUrl.h"
#include "../libraries/AS/include/AS/algorithms.h" // use Algorithms
#include "../libraries/AS/include/AS/array.h"

#include <stdlib.h> /* atoi */
#include <zlib.h> // deflate
#include <iostream>
using APESEARCH::unique_ptr;


Request::Request() : headerBuff( 16384, 0 ), isHtml( false ) {}

/*
 * REQUIRES: Nothing
 * MODIFIES: headerBuff
 *  EFFECTS: Receives all of the header from a Request to a server and returns where the header ends (via \r\n\r\n)
 *           and how much further from the end of the header via the second argument in pair. This is required since
 *           part of the body can already be receieved and it may be necessary to still process it (gzip, chunked) before
 *           placing it into bodyBuff.
 * 
 *    Effectively analyzes every character received and reads into headerBuff 
 *    until the end of header character pattern is reached( \r\n\r\n ).
*/
APESEARCH::pair< char const * const, char const * const > Request::getHeader( unique_ptr<Socket> &socket )
    {
    int bytesReceived = 0;
    static char const * const endHeader = "\r\n\r\n";
    char const *place = endHeader;
    
    char *bufPtr, *headerEnd;
    headerEnd = bufPtr = headerBuff.begin();

    while( *place && headerEnd < headerBuff.end( ) && 
      ( bytesReceived = socket->receive( bufPtr, static_cast<size_t> ( headerBuff.end() - bufPtr ) ) ) > 0 )
        {
        headerEnd += bytesReceived;
        while( *place && bufPtr != headerEnd )
            (*place == *bufPtr++) ? ++place : place = endHeader;

        } // end while
   // Reached the end of header

   //std::cout << APESEARCH::string(  &headerBuff.front(), bufPtr ) << std::endl;

   return APESEARCH::pair< char const * const, char const * const  > 
      ( ( *place ? nullptr : bufPtr ), ( *place ? nullptr : headerEnd ) );
    } 


/*
 * REQUIRES: urlStr's number of bytes to be at most 870 bytes. This is a temporary fix to account for urls that may be larger than
 *           buf can hold (max bytes being 1024).
 * MODIFIES: headerBuff through getHeader and bodyBuff through getBody.
 *  EFFECTS: Attempts to do a get request to the url passed in as urlStr. 
 *          
 *    It first parses the url to obtain different entries like 1) the host in order to perform DNT ( domain name translation )
 *    2) Service, to get the right protocol (http or https), etc.
 *    
 *    Next, 
 *           
*/
Result Request::getReqAndParse(const char *urlStr)
{
   // Prevent bufferoverflow
   if ( strlen( urlStr ) > 870 )
      return Result( getReqStatus::badURL );
    urlPtr= urlStr;
    ParsedUrl url( urlStr );
    bool httpProtocol = !strcmp(url.Service, "http"); // 0 if http 1 if https
    Address address(url.Host, *url.Port ? url.Port : httpProtocol ? "80" : "443" );
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
         // Combine Request header and fields
         snprintf( buff, sizeof( buff ), "GET /%s HTTP/1.1\r\nHost: %s\r\n%s", url.Path, url.Host, fields );
         //printf( "%s\n", buff );
         //! Start timer here...
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
      // HTTP/1.x <Resp Num>" "
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
      case statusCategory::successful:

         return getReqStatus::successful;
      case statusCategory::redirection:

         return getReqStatus::redirected;
      case statusCategory::serverError:

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
      return; // Was the wrong field (do nothing)

   // Found the key, now processing value
   const char *endVal;
   while ( ( endVal = APESEARCH::findChars( headerPtr, endOfLine, fieldTerm ) ) != endOfLine )
      {
      valueProcess( headerPtr, endVal++ );
      headerPtr = endVal;
      } // end while
   } // end processField

/*
* REQUIRES: endOfHeader to indeed point to the end of a header
*/
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
      redirect = true;

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
                  int digit = *front++ - '0';
                  if ( digit < 0 || digit > 10 )
                     {
                     contentLengthBytes = 0;
                     headerBad = true;
                     return;
                     } // end if
                  contentLengthBytes *= 10;
                  contentLengthBytes += std::size_t ( digit );
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
               if ( headerBad || contentLengthBytes > Request::maxBodyBytes )
                  {
                  resultOfReq.status = getReqStatus::badHtml;
                  return resultOfReq;
                  } // end if
               } // end if
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
      // Update pointer to point to next line
      headerPtr = endOfLine;
      } // end while
   return resultOfReq;
}

static void DecompressResponse( APESEARCH::vector < char >& data_ );
void Request::getBody( unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody )
   {
   
   if ( chunked )
      chunkedHtml( socket, partOfBody );
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

void Request::receiveNormally( unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody )
   {
   bodyBuff.resize( contentLengthBytes );
   char *bufPtr = APESEARCH::copy( partOfBody.first( ), partOfBody.second( ), &bodyBuff.front( ) );
   char *bufEnd = &bodyBuff.front( ) + contentLengthBytes;

   while( bufPtr < bufEnd )
      bufPtr += socket->receive( bufPtr, static_cast<int>( bufEnd - bufPtr ));

   }  // end normalHtml( )


static ssize_t hexaToDecimal( char const *begin, char const *end )
   {
   ssize_t num = 0;
   while( begin != end )
      {
      int hexa;
      char character = *begin++;
      if ( '0' <= character && character <= '9')
         hexa = character - '0';
      else if ( 'a' <= character && character <= 'f' )
         hexa = character - 'a' + 10;
      else if ( 'A' <= character && character <= 'F' )
         hexa = character - 'A' + 10;
      else
         return -1;
      num <<= 4; // was moved after branches since num<<=4 is only required 
                 //after it reaches any condition other than the else
      num += hexa;
      } // end while
   return num;
   }

/*
 * REQUIRES: socket points to an actual socket.
 * MODIFIES: buffer
 *  EFFECTS: Seeks for the delimiter siginifying the end of a chunk (be it the chunk size or the end of a chunk).
 *           Utilizes a buffer to hold the necessary data and returns a pointer to the start of it. This is useful
 *           to find out the chunk size. 
 *           Due to the limit of the size and that shifting bytes is limited as much as possible, 
 *           it may be necessary to shift the valid data to the beginning of the buffer were the
 *           pointer to the end reaches the end of the buffer itself (buffer.end()) in order for Request to continue being able
 *           to receive data. The reason for this is because, the buffer is reused so when receving the chunk itself, it may be possible that the
 *           next chunk size signifier/chunk size may already have been received; a choice made is to avoid shifting bytes as much as possible until it is 
 *           absolutely necessary. With a size of 2^16 bytes buffer, the need of shifting should be limited as much as possible.
 *           
 *           i.e [ R, R, R, R, 1, F, e, 0, \r ] => [ 1, F, e, 0, \r, R, R, R, R ]
 *           *** R signifies bytes that don't matter
*/
inline const char *seekLineSeperator(  unique_ptr<Socket> &socket, char ***ptr, char const ***currEnd, APESEARCH::vector<char>& buffer )
   {
   static char const * const endChunkSize = "\r\n";
   char const *start = **ptr;
   char const *place = endChunkSize;
   ssize_t bytesReceived = 0;
   assert( **ptr <= **currEnd );

   do
   {
      **currEnd += bytesReceived;
      while( **ptr != **currEnd )
         {
         if ( * ( **ptr )++ == *place )
            {
            if ( !( *++place ) )
               return start;
            } // end if
         else
            place = endChunkSize;
         } // end while

      // Check if we're currently at the end of buffer
      if ( **ptr == buffer.end( ) )
         {
         // Cannot shift any further so just stops reading in anything
         if ( buffer.end( ) - start == buffer.size( ) )
            break;

         // Copy buffer to the beginning shifting defined bytes to the beginning to continue reading in
         char *retPtr = APESEARCH::copy( start, ( const char * ) buffer.end( ), buffer.begin( ) );
         assert( ( retPtr - buffer.begin( ) ) == ( buffer.end( ) - start ) );

         // Readjust pointers
         start = buffer.begin( ); // start now begins at the beginning
         **currEnd = (const char *) ( **ptr = retPtr );
         } // end if
   } while ( ( bytesReceived = socket->receive( **ptr, buffer.end( ) - **ptr ) ) > 0 );
   return nullptr;
   }

ssize_t Request::findChunkSize( unique_ptr<Socket> &socket, char **ptr, char const **currEnd, APESEARCH::vector<char>& buffer )
   {
   char const *start = *ptr;

   if ( !( start = seekLineSeperator( socket, &ptr, &currEnd, buffer ) ) )
      return -1;

   // Invariant assertion to ensure that the previous two bytes are \r\n respectively
   if ( *(*ptr - 1) != '\n' || *(*ptr - 2) != '\r' || start >= *ptr )
      {
      char const *iterator = buffer.begin( );
      while( iterator != *currEnd )
         std::cout << *iterator++;
      printf("\nIssue with Url: %s\n", urlPtr);
      assert( *(*ptr - 1) == '\n' && *(*ptr - 2) == '\r' && start < *ptr );
      } // end if

   return hexaToDecimal( start, *ptr - 2 );
   } // end findChunkSize( )

bool Request::attemptPushBack( char val )
   {
   // If about to increase, check if would go over
   if ( bodyBuff.size( ) == bodyBuff.capacity( ) && bodyBuff.capacity( ) << 1 > Request::maxBodyBytes )
      return false;
   bodyBuff.push_back( val );
   return true;
   } // end val

// Continously iterate through the buffer and write to bodyBUff until bytes
bool Request::writeChunked( unique_ptr<Socket>& socket, APESEARCH::vector<char>& buffer, char **ptr, char const**currEnd, const size_t bytesToReceive )
   {
   assert( bytesToReceive > 0 );
   assert( *ptr <= *currEnd );
   ssize_t bytesToWrite = 0;
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
            char const *start = seekLineSeperator( socket, &ptr, &currEnd, buffer );
            // Expects that immediately after is \r\n (so the difference should be exactly 2)
            return start && *ptr - start == 2;
            } // end if
         } // end while
      // Point to the beginning of the buffer.
      *currEnd = static_cast<const char *>( *ptr = buffer.begin( ) );
   } while( ( bytesToWrite = socket->receive( *ptr, buffer.end( ) - *ptr ) ) > 0 );
   return false;
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
   // Reached the end ( no longer receiving )
   if ( chunkSize <= 0 )
      {
      headerBad = chunkSize == -1;
      return;
      } // end if
   
   // Now attempt to continue to write into BodyBuff
   if ( !writeChunked( socket, temp, &buffPtr, &currEnd, static_cast<size_t>( chunkSize ) ) )
      {
      headerBad = true;
      return;
      } // end if
   } while ( true );
   } // end chunkedHtml( )





// Perform the decompression
void DecompressResponse( APESEARCH::vector < char >& data_ )
    {
    z_stream zs;                        // z_stream is zlib's control structure
    memset( &zs, 0, sizeof( zs ) );

    APESEARCH::vector < char > decompressed;
    APESEARCH::array < char, 32768 > outBuffer;

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
