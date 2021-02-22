#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <iostream>
#include "assert.h"

class ParsedUrl
   {
   public:
      const char *CompleteUrl;
      char *Service, *Host, *Port, *Path;

      ParsedUrl( const char *url )
         {
         // Assumes url points to static text but
         // does not check.

         CompleteUrl = url;

         pathBuffer = new char[ strlen( url ) + 1 ];
         const char *f;
         char *t;
         for ( t = pathBuffer, f = url; ( *t++ = *f++); )
            ;

         Service = pathBuffer;

         const char Colon = ':', Slash = '/';
         char *p;
         for ( p = pathBuffer; *p && *p != Colon; p++ )
            ;

         if ( *p )
            {
            // Mark the end of the Service.
            *p++ = 0;

            if ( *p == Slash )
               p++;
            if ( *p == Slash )
               p++;

            Host = p;

            for ( ; *p && *p != Slash && *p != Colon; p++ )
               ;

            if ( *p == Colon )
               {
               // Port specified.  Skip over the colon and
               // the port number.
               *p++ = 0;
               Port = +p;
               for ( ; *p && *p != Slash; p++ )
                  ;
               }
            else
               Port = p;

            if ( *p )
               // Mark the end of the Host and Port.
               *p++ = 0;

            // Whatever remains is the Path.
            Path = p;
            }
         else
            Host = Path = p;
         }

      ~ParsedUrl( )
         {
         delete[ ] pathBuffer;
         }

   private:
      char *pathBuffer;
   };

int main( int argc, char **argv )
   {

   if ( argc != 2 )
      {
      std::cerr << "Usage: " << argv[ 0 ] << " url" << std::endl;
      return 1;
      }

   // Parse the URL
   ParsedUrl url( argv[ 1 ] );

   // Get the host address.
   struct addrinfo *address, hints;
   memset(&hints, 0, sizeof(hints));
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_protocol = IPPROTO_TCP;

   getaddrinfo( url.Host, *url.Port ? url.Port : "80", &hints, &address );
   // Create a TCP/IP socket.
   int socketFD = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

   // Connect the socket to the host address.
   int connectResult = connect( socketFD, address->ai_addr, address->ai_addrlen );
   // Send a GET message.

   char *getRequest;

   //! Hey Paul you simply forgot to include the path in your get request.
   int size = asprintf( &getRequest, "GET /%s HTTP/1.1\r\nHost: %s\r\nUser-Agent: LinuxGetUrl/2.0 paulzhan@umich.edu (Linux)\r\n\
   Accept: */*\r\n Accept-Encoding: identity\r\nConnection: close\r\n\r\n", url.Path, url.Host );

   if (size == -1 ) {
      perror("Error with asprintf, not enough memory\n");
      close(socketFD);
      freeaddrinfo(address);
      return 1;
   } 

   send( socketFD, getRequest, size, 0 );
   free( getRequest );
   // Read from the socket until there's no more data, copying it to
   // stdout. 

   //! In addition, here's our method of removing the response header and reading the rest to stdout.
   char buffer[10240];
   int bytesToWrite;
   static char const * const endHeader = "\r\n\r\n";

   char const *place = endHeader;
   while((bytesToWrite = recv(socketFD, buffer, sizeof(buffer), 0)) > 0)
   {  
      char *bufPtr, *bufEnd;
      bufEnd = ( bufPtr = buffer ) + bytesToWrite;

      while(*place && bufPtr != bufEnd)
      { 
         assert(bytesToWrite > 0);
         (*place == *bufPtr++) ? ++place : place = endHeader;
         --bytesToWrite;
      } // end while
      assert ( bytesToWrite >= 0 );
      if( !*place && bytesToWrite ) 
      {
         write(1, bufPtr, bytesToWrite);
      }
   } // end while


   // Close the socket and free the address info structure.
   close(socketFD);
   freeaddrinfo(address);

   }
