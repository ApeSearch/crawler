#include "Address.h"
#include "ParsedUrl.h"
#include <openssl/ssl.h>
#include <iostream>
#include <unistd.h>
#include "GetRequest.h"

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

#define MAXATTEMPTS 5

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
   
   //Socket *socket = strcmp(url.Service, "http://") ? new Socket(address, TIMEOUT) : new SSLSocket(address, TIMEOUT);
   
   //TODO
   // Maybe use another addrinfo in linked-list if this timesout 
   // Create a TCP/IP socket.
   // Connect the socket to the host address.
   
   // Send a GET message.
   std::string getMessage = url.formRequest();
   int attempts = 0;


   while(attempts < MAXATTEMPTS)
   {
      try
      {
         GetRequest req(url, getMessage, address);
         req.parseRequest();
         break;
      }
      catch(...)
      {
         //std::cout << "send failed" << endl; 
         //TODO Update address to loop around back to the beginning
         if(address.info->ai_next)
            address.info = address.info->ai_next;
         ++attempts;
      }

   }

  }
