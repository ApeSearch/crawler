
#include "../include/crawler/Mercator.h"
#include <signal.h> 
#include <fstream>
#include <signal.h>
#include "../include/crawler/SSLSocket.h"
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define MAXNODES 12
// ./exec <NODE_ID> 
// ./exec 1 < seed_list.txt
//34.201.187.203 ( 1 )
int main( int argc, char **argv )
    {
    // Initialize SSLlibrary
    SSL_library_init( );
    //SIGPIPE setting
    signal(SIGPIPE, SIG_IGN);

    //if ( node_id < 0 || node_id > MAXNODES  )
    //   {
    //   std::cerr << "Node id is out of bounds: " << node_id << std::endl;
    //   return 1;
    //   } // end if
    
    APESEARCH::vector<Link> seed_links;
    std::string url;

    std::ifstream in( "./crawling_list.txt" );
    
    while( in >> url )
        {
        Link link;
        link.URL = APESEARCH::string(url.begin(), url.end());
        seed_links.push_back( link );
        } // end while

    
    //Nikolas1, Nikolas2, Robin1, Robin2, Paul1, Paul2, Christian1, Christian2 Serdar1, Serdar2, Alex1, Alex2
    APESEARCH::vector<APESEARCH::string> ips = { "35.230.41.55", "34.71.229.2", "34.75.57.124", "35.245.134.242", "35.194.60.3", "35.232.126.246", "199.223.236.235", "35.194.73.46", "35.231.170.57", "34.86.240.51", "34.73.221.32", "34.86.225.197" };
    if(ips.size() != MAXNODES)
        {
        std::cerr << "Wrong amount of ips: " << ips.size() << std::endl;
        return 2;
        } // end if

    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in *sa;
    char *addr;
    int node_id = -1;

    getifaddrs (&ifap);
    for (ifa = ifap; ifa && node_id == -1; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family==AF_INET) {
            sa = (struct sockaddr_in *) ifa->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);
            
            for(int i =0; i < ips.size(); ++i)
                {
                if( !strcmp( ips[ i ].cstr( ), addr ) )
                    {
                        std::cerr << "Starting to run on this ip: " << ips[ i ] <<  " this is Node " << i << "\n";
                        node_id = i;
                        break; 
                    }                    
                }
        }
    } // end for
    freeifaddrs(ifap);
    if( node_id < 0)
    {
        std::cerr << "Could not find your local ip address\n";
        return 3;
    }
    
    // crawlers, parsers
    //APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, 768, 384, 0, seed_links);
    
    //merc.user_handler( );
    return 0;
    } // end main( )
