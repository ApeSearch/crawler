#include <string.h>
#include <unistd.h>
#include <string>
#include <atomic>
#include <iostream>
#include "assert.h"
#include <netdb.h> // For addrinfo
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>

/*
class Crawler
{
   public:


   private:
      ParsedUrl parsedUrl;
      OutputFileDescriptor;
      Address *address;
      char *userAgent;
};
*/

// RAII wrapper for addrinfo struct
struct Address {
    Address() {}

    // ai_family specifies the address family: which restricts the kind of addresses to the same type.
    // i.ei AF_UNIX are UNIX sockets, AF_IPS, IPX. Bluetooh has AF_BLUETOOTH.
    // AF_INET6 is for v6 addresses (Use AF_INET as it's the safest option)
    Address(char *Host, char *Port, int addrFamily = AF_INET, 
          int sockType = SOCK_STREAM, int transportProtocol = IPPROTO_TCP) 
    {
        memset( &hints, 0, sizeof( hints ) );
        hints.ai_family = addrFamily;
        hints.ai_socktype = sockType;
        hints.ai_protocol = transportProtocol;
        // If this does not return 0, the DNS was unable to resolve the hostname:port
        // and so we can assume it's invalid
        if (getaddrinfo( Host, (Port == nullptr) ? "80": Port, &hints, &info ) != 0 ) {
            perror("Error inside Address Constructor occured");
        } // end if
    }

    ~Address() 
    {
        if (valid)
            freeaddrinfo( info );
    }

    struct addrinfo *info, hints;
    bool valid;

    // PrintAddressInfo as defined in class
    //Used only in regression testing methods
    //friend std::ostream &operator<<( std::ostream &os, const Address & addr);
}; // end Address


class Socket 
{
   public:
      Socket(const Address& address) 
      {
         socketFD = socket(address.hints.ai_family, address.hints.ai_socktype, address.hints.ai_protocol);
         connectResult = connect(socketFD, address.info->ai_addr, address.info->ai_addrlen);
      }
      virtual ~Socket() 
      {
         if (socketFD != -1) 
            close(socketFD);  
      }

      virtual ssize_t send(const char* buffer, int length) 
      {
         if (socketFD == -1) 
         {
            throw;
         }
         ::send(socketFD, buffer, length, 0);
      }

      virtual ssize_t receive(char *buffer, int length)
      {
         if(socketFD == -1)
         {
            throw;
         }
         return ::recv(socketFD, buffer, length, 0);
      }

   protected:
      struct timeval tv;
      int socketFD;
      int connectResult;
};

class SSLSocket : public Socket 
{
   public:
   SSLSocket(const Address& address) : Socket(address) 
   {
      // Initialize the SSL Library
      sslFramework = SSL_CTX_new(SSLv23_method());
      ssl = SSL_new(sslFramework);
      SSL_set_fd(ssl, socketFD);
      SSL_connect(ssl);
   }
   ~SSLSocket() 
   {
      SSL_shutdown(ssl);
      SSL_free(ssl);
      SSL_CTX_free(sslFramework);
   }
   ssize_t send(const char* buffer, int length)
   {
      return SSL_write(ssl , buffer, length);
   }
   ssize_t receive(char* buffer, int length)
   {
      return SSL_read(ssl, buffer, length);
   }
   private:
      // Pointerto to a framework which establishes TLS/SSL enabled ocnnections
      SSL_CTX * sslFramework;
      SSL * ssl;
}; // end SSLSocket


class Header 
{
   public:
      //Make sure response header is 404, if its 302 we must redirect.
      std::string response;
      std::string date;
      std::string server;
      std::string last_modified;
      std::string content_length;
      std::string content_type;
      //Check content_encoding for gzip
      std::string content_encoding;

      Header()
      {

      }
      ~Header()
      {
         
      }
   private:

};

class ParsedUrl
   {
   public:
      const char *CompleteUrl;
      char *Service, *Host, *Port, *Path;
      bool protocolType; // true == http, false == https

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
            protocolType = strcmp(Service, "https");

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

   SSL_library_init();

   if ( argc != 2 )
      {
      std::cerr << "Usage: " << argv[ 0 ] << " url" << std::endl;
      return 1;
      }

   // Parse the URL
   ParsedUrl url( argv[ 1 ] );
   
   // Get the host address.
   Address address(url.Host, "80"); 
   
   Socket *socket = strcmp(url.Service, "http://") ? new Socket(address) : new SSLSocket(address);
   //TODO
   // Maybe use another addrinfo in linked-list if this timesout 
   // Create a TCP/IP socket.
   // Connect the socket to the host address.

   // Send a GET message.
   std::string getMessage = url.formRequest();
   try{
      socket->send(getMessage.c_str(), getMessage.length());
   }
   catch(...){
      //std::cout << "send failed" << endl; 
   }
   

   // Read from the socket until there's no more data, copying it to
   // stdout. 
   char buffer[10240];
   int bytesToWrite;
   static char const * const endHeader = "\r\n\r\n";


   char const *place = endHeader;
   while((bytesToWrite = socket->receive(buffer, 10240)) > 0)
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

   delete socket;
  }
