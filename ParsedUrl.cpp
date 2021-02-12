#include <sstream>
#include "ParsedUrl.h"

ParsedUrl::ParsedUrl( const char *url ) : CompleteUrl(url), valid_url(false) {
    parseUrl();
}

ParsedUrl::~ParsedUrl() {}

void ParsedUrl::parseUrl() {
    // There shouldn't be a space encoded in the URL (maybe make this catch all invalid chars?)
    if (handle_error(strchr(CompleteUrl, ' ') != nullptr))
        return;

    // Retrieve the Service Component, verifying that it's present and a valid service
    Service = getUrlComponent(CompleteUrl, ':');
    if (handle_error(!validService(Service.get())))
        return;
        
    // Get pointers to the start and end of hostname (this can also contain :PORT, which is dealt with in the next block)
    auto hostStart = CompleteUrl + strlen(Service.get()) + 3;
    auto hostEnd = strchr(hostStart, '/');
    // start of hostname must be alphanumeric
    if (handle_error(!isalnum(*hostStart)))
        return;
    
    // Deal with the URL where there's a port number after the hostname but in between the path
    auto portStart = strchr(hostStart, ':');
    if (portStart != nullptr && portStart < hostEnd) 
        {
            auto portEnd = hostEnd;
            hostEnd = portStart;
            
            // We want portStart to begin at the number, not the colon in front of it (hence portStart + 1)
            Port = constructComponent(portStart + 1, portEnd);
        }

    // Case where there is a '/' at the end of the hostname, and potentially a path (i.e. https://www.facebook.com/index.html)
    if (hostEnd) {
        Host = constructComponent(hostStart, hostEnd);
        if (handle_error(!isalnum(*(hostEnd - 1))))
            return;

        // Retrieve the Path component
        auto pathStart = (Port) ? (hostEnd + strlen(Port.get()) + 2) : (hostEnd + 1);
        Path = getUrlComponent(pathStart, '\0');
    }
    // Case where there is no '/' at the end of the hostname (i.e. https://www.facebook.com)
    else {
        Host = getUrlComponent(hostStart, '\0');
    }    
    //  end of hostname should be alphanumeric (TODO: verify this is always true)
    
} // end parsedUrl()

// Constructs a component (Service, Host, Path, Port) of a URL given a start location and a symbol to parse until
std::unique_ptr<char[]> ParsedUrl::getUrlComponent(const char *start, char symbol) {
    // Find the first occurence of the symbol; this is the end of the component we're looking for
    // (e.g. there will always be a colon at the end of the Service component https://)
    auto end = strchr(start, symbol);
    if (end == nullptr || start == end) return nullptr;
    return constructComponent(start, end);
} // end getUrlComponent()

// Creates a new unique_ptr to a cstring representing a URL component (i.e. Port, Service, Hostname, Path)
std::unique_ptr<char[]> ParsedUrl::constructComponent(const char *start, const char *end) {
    auto component = std::make_unique<char[]>(end- start + 1);
    strncpy(component.get(), start, end - start);
    return component;
} // end constructComponent()

// Forms a GET request to the Host and Path
std::string ParsedUrl::formRequest() 
    {
    std::ostringstream stream;

    stream << "GET /" << Path.get() << " HTTP/1.1\r\n";
    stream << "Host: " << Host.get() << "\r\n";
    stream << "User-Agent: " << "LinuxGetUrl/ username@email.com" << "\r\n";
    stream << "Accept: */*\r\n";
    stream << "Accept-Encoding: identity\r\n";
    stream << "Connection: close\r\n";
    stream << "\r\n";

    return stream.str();
    } // end formRequest()

// Print out information associated with an Address (information from getaddrinfo)
// this was defined in class as PrintAddressInfo, but << is neater imo
std::ostream &operator<<( std::ostream &os, const Address & addr) {
   auto s = ( sockaddr_in* ) addr.info->ai_addr;
   const struct in_addr *ip = &s->sin_addr;
   uint32_t a = ntohl( ip->s_addr );
   os << "Host address length = " << sizeof( struct sockaddr) << " bytes" << std::endl;
   os << "Family = " << s->sin_family << ", port = "<< ntohs( s->sin_port) 
               << ", address = "<< ( a >> 24 ) << '.' <<( ( a >> 16 ) & 0xff ) 
               << '.' << ( ( a >> 8 ) & 0xff ) << '.' << ( a & 0xff ); 
   return os;
}