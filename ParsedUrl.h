#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <stdio.h>

// We'll have to define our own for these I think
#include <string.h>
#include <string>
#include <memory>

// RAII wrapper for addrinfo struct
struct Address {
    Address() {}
    Address(char *Host, char *Port) : Address(Host, Port, AF_INET, SOCK_STREAM, IPPROTO_TCP)  {}

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
    friend std::ostream &operator<<( std::ostream &os, const Address & addr);
}; // end Address
    
class ParsedUrl {
public:
    const char *CompleteUrl; // http://localhost:5000/index.htm

    std::string Protocol;
    std::unique_ptr<char[]> Service; // http:
    std::unique_ptr<char[]> Host;    // localhost
    std::unique_ptr<char[]> Port;    // 5000
    std::unique_ptr<char[]> Path;    // index.htm

    char *Service, *Host, *Port, *Path;
    char *pathBuffer;


    bool valid_url; // Was the constructor given a well-formed URL?

    ParsedUrl( const char *url );
    ~ParsedUrl( );

private:
    std::unique_ptr<char[]> constructComponent(const char *start, const char *end);
    std::unique_ptr<char[]> getUrlComponent(const char *start, char symbol);
    void parseUrl();

    // This probably doesn't belong in this class but it's here for now
    std::string formRequest();

    bool handle_error(const char *ptr) {
        valid_url = (ptr != nullptr) ? true: false;
        return ptr == nullptr;
    }

    bool handle_error(bool condition) {
        valid_url = !condition ? true: false;
        return condition;
    }

    bool validService(const char *service) 
        {
            if (service == nullptr) return false;
            return strcmp(service, "http") == 0 || 
                    strcmp(service, "https") == 0 || 
                    strcmp(service, "ftp") == 0;
        }
}; // end ParsedUrl
