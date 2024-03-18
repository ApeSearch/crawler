#include "../include/crawler/SSLSocket.h"

SSLSocket::SSLSocket(const Address& address, time_t seconds) : Socket(address, seconds) 
{
   // Initialize the SSL Library
   sslFramework = SSL_CTX_new(SSLv23_method());
   ssl = SSL_new(sslFramework);
   SSL_set_fd(ssl, socketFD);
   SSL_connect(ssl);
}

SSLSocket::~SSLSocket() 
{
   SSL_shutdown(ssl);
   SSL_free(ssl);
   SSL_CTX_free(sslFramework);
}

ssize_t SSLSocket::send(const char* buffer, int length)
{
   return SSL_write(ssl , buffer, length);
}
ssize_t SSLSocket::receive(char* buffer, int length)
{
   return SSL_read(ssl, buffer, length);
}
