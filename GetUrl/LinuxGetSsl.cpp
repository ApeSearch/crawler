#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
//#include <openssl/ssl.h>
#include <string>

#include <iostream>
#include "assert.h"

class ParsedUrl
   {
   public:
      const char *CompleteUrl;
      char *Service, *Host, *Port, *Path;

      std::string formRequest() 
      {
         std::string header;
         header.reserve(50);
         // Get req
         header += "GET /";
         header.append(Path); 
         
         header += " HTTP/1.1\r\n";
         
         header += "Host: ";
         header.append(Host);
         header += "\r\nUser-Agent: LinuxGetUrl/ username@email.com\r\nAccept:";
         header += "*/*\r\nAccept-Encoding: identity\r\nConnection: close\r\n\r\n";
         return header;
      } // end formRequest()


      ParsedUrl( const char *url )
         {
         // Assumes url points to static text but
         // does not check.

         CompleteUrl = url;

         pathBuffer = new char[ strlen( url ) + 1 ];
         const char *f;
         char *t;
         for ( t = pathBuffer, f = url; *t++ = *f++; )
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
   
   //TODO
   // Maybe use another addrinfo in linked-list if this timesout 
   getaddrinfo(url.Host, "80", &hints, &address);

   // Create a TCP/IP socket.
   int socketFD = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

   // Connect the socket to the host address.
   int connectResult = connect(socketFD, address->ai_addr, address->ai_addrlen);

   // Send a GET message.
   std::string getMessage = url.formRequest();
   send(socketFD, getMessage.c_str(), getMessage.length(), 0);

   // Read from the socket until there's no more data, copying it to
   // stdout. 
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
   freeaddrinfo( address );
  }
