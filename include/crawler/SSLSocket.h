#pragma once

#ifndef SSLSOCKET_H_APESEARCH
#define  SSLSOCKET_H_APESEARCH

#include "../../libraries/AS/include/AS/Socket.h"
#include <openssl/ssl.h>
//#include "/usr/local/opt/openssl/include/openssl/ssl.h"

class SSLSocket : public Socket
{
   public:
     SSLSocket(const Address& address, time_t seconds);
      
     virtual ~SSLSocket();

     ssize_t send(const char* buffer, int length) override;

     ssize_t receive(char *buffer, int length) override;

   protected:
    // Pointerto to a framework which establishes TLS/SSL enabled ocnnections
      SSL_CTX * sslFramework;
      SSL * ssl;
};


#endif