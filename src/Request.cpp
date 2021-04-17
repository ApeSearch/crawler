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

    while( *place && ( bytesReceived = socket->receive( bufPtr, static_cast<size_t> ( &*headerBuff.end() - bufPtr ) ) ) > 0 )
        {
        headerEnd += bytesReceived;
        while( *place && bufPtr != headerEnd )
            (*place == *bufPtr++) ? ++place : place = endHeader;
        //TODO Some statistics tracking to see how many times this runs

        } // end while
   
   //construct string based off of buffer call our pase header function
   // Reached the end of header

   std::cout << std::string(  &headerBuff.front(), bufPtr ) << std::endl;
   return APESEARCH::pair< char const * const, char const * const  > 
      ( ( *place ? nullptr : bufPtr ), ( *place ? nullptr : headerEnd ) );
    } 


/*
 * Where the request is done
*/
Result Request::getReqAndParse(const char *urlStr)
{
    ParsedUrl url( urlStr );
    Address address(url.Host, "80");
    APESEARCH::pair<const char *, size_t> req = url.getReqStr();
    bool httpProtocol = !strcmp(url.Service, "http://"); // 0 if http 1 if https
    int attempts = 0;
    Result res;
    while(attempts < MAXATTEMPTS)
      {
      try
         {
         unique_ptr<Socket> socket( httpProtocol ? new Socket(address, Request::timeoutSec) : new SSLSocket(address, timeoutSec) );
         char buff[1024];
         snprintf( buff, sizeof( buff ), "%s%s", req.first( ), fields );
         printf( "User Agent: %s\n", buff );
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
            || foundContentLength && (contentLengthBytes > Request::maxBodyBytes || headerPtrs.second( ) - headerPtrs.first( ) > ( ssize_t ) contentLengthBytes ))
            {
            res.status = getReqStatus::badHtml;
            return res;
            } // end if

         getBody( socket, headerPtrs );
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

      std::cout << "Response: " << status << std::endl;
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
            break;
            } // end case 'C'
         case 'L':
            {
            auto Lpred = [&resultOfReq, this]( char const * front, char const *end ) 
               {  
               resultOfReq.url = std::string( front, end );
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
   bodyBuff.resize( contentLengthBytes );
   char *bufPtr = APESEARCH::copy( partOfBody.first( ), partOfBody.second( ), &bodyBuff.front( ) );
   char *bufEnd = &bodyBuff.front( ) + contentLengthBytes;

   while( bufPtr < bufEnd )
      bufPtr += socket->receive( bufPtr, static_cast<int>( bufEnd - bufPtr ));

   }  // end normalHtml( )

//size_t ctoh()

static ssize_t hexaToDecimal( char const *begin, char const *end )
   {
   size_t num = 0;
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
char const *getEndOfChunked( unique_ptr<Socket> &socket, char *bufferStart, char * bufferTrueEnd )
   {      
    size_t totBytesTransfered = 0;
    int bytesReceived = 0;
    static char const * const endChunked = "\r\n\r\n";
    char const *place = endChunked;

    char *bufPtr, *bufEnd;
    bufEnd = bufPtr = bufferStart;

    while( *place && ( bytesReceived = socket->receive( bufPtr, static_cast<int>( bufferTrueEnd - bufPtr ) ) ) ) 
      {
      bufEnd += bytesReceived;
      totBytesTransfered += bytesReceived;
      while( *place && bufPtr != bufEnd )
         (*place == *bufPtr++) ? ++place : place = endChunked;
      
      if ( totBytesTransfered >= bufferTrueEnd - bufferStart )
         return bufferTrueEnd;
      } // end while
   }

static char const *findSubStr( char const *begin, char const *end, char const * const substr )
   {
   char const *ptr = begin;
   const char * at = substr;

   while ( *at && ptr != end )
      *ptr++ == *at ? ++at : at = substr;
   return !( *at ) ? begin + ( static_cast<size_t>( ptr - begin ) - strlen( substr ) ) : end;
   } // end findPtr()

void Request::chunkedHtml(unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody)
{
   static constexpr char * const newline = "\r\n";
   APESEARCH::vector<char> temp(131072);

   size_t chunksToRead = 0;
   char *bufPtr =  &temp.front( );
   char *bufEnd = APESEARCH::copy( partOfBody.first( ), partOfBody.second( ), bufPtr );
   char const *endOfChunk;
   char const * cbufEnd = getEndOfChunked( socket, bufEnd, temp.end( ) );

   while( ( endOfChunk = findSubStr( bufPtr, cbufEnd, newline ) ) != cbufEnd )
      {         
      size_t chunksToRead = hexaToDecimal( bufPtr, endOfChunk );
      if ( chunksToRead == 0 )
         {
         if ( !strCmp( endOfChunk, endOfChunk + 4, "\r\n\r\n" ) )
            headerBad = true;
         return;
         } // end if
      assert( ( endOfChunk[ 0 ] == '\r' ) );
      assert( ( endOfChunk[ 1 ] == '\n' ) );
      // skip past \r\n
      bufPtr = ( char * ) endOfChunk + 2;

      // Now push up to chunksToRead
      size_t bytesInputted = 0;
      endOfChunk = bufPtr + chunksToRead;
      while( bufPtr != endOfChunk )
         {
         bodyBuff.push_back( *bufPtr++ );
         ++bytesInputted;
         } // end while
      // Skip \r\n
      if ( *bufPtr++ != '\r' || *bufPtr++ != '\n' )
         {
         headerBad = true;
         return;
         } // end if
      } // end while
}

//static void DecompressResponse( APESEARCH::vector < char >& data_ );

void Request::getBody( unique_ptr<Socket> &socket, APESEARCH::pair< char const * const, char const * const >& partOfBody )
   {
   
   if ( chunked )
      chunkedHtml( socket, partOfBody );
   else
      receiveNormally( socket, partOfBody );
      
   if ( gzipped )
      //DecompressResponse( bodyBuff );

   return;
   }

   APESEARCH::vector< char > Request::getResponseBuffer()
   {
      return std::move( bodyBuff );
   }


/*
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
*/