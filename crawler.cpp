
#include "include/crawler/Mercator.h"
#include "include/crawler/SSLSocket.h"
#include <signal.h> // For SIGPIPE
#include <fstream>
#include <signal.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define MAXNODES 2
int main( int argc, char **argv )
    {
    // Initialize SSLlibrary
    SSL_library_init( ); // Initialize relevant resources
    //SIGPIPE setting
    signal(SIGPIPE, SIG_IGN); // Prevents crashing if SIGPIPE signal occurs
    
    printf("IP address: %s\n", argv[1]);

    APESEARCH::vector<Link> seed_links;
    std::string url;

    std::ifstream in( "./crawling_list.txt" );
    
    // Input urls from crawling list into url vector
    while( in >> url )
        {
        seed_links.emplace_back( APESEARCH::string(url.begin(), url.end()) );
        } // end while

    
    //Nikolas1, Nikolas2, Robin1, Robin2, Paul1, Paul2, Christian1, Christian2 Serdar1, Serdar2, Alex1, Alex2
    //APESEARCH::vector<APESEARCH::string> ips = { "35.230.41.55", "34.71.229.2", "34.75.57.124", "35.245.134.242", "35.194.60.3", "35.232.126.246", "199.223.236.235", "35.194.73.46", "35.231.170.57", "34.86.240.51", "34.73.221.32", "34.86.225.197" };
    APESEARCH::vector<APESEARCH::string> ips = { "34.75.57.124", "35.245.134.242" };
    if(ips.size() != MAXNODES)
        {
        std::cerr << "Wrong amount of ips: " << ips.size() << std::endl;
        return 2;
        } // end if

    int node_id = -1;
    APESEARCH::string ip( argv[ 1 ] );
    for(int i =0; i < ips.size(); ++i)
        {
        if( ips[ i ] == ip ) 
            {
            std::cerr << "Starting to run on this ip: " << ips[ i ] <<  " this is Node " << i << "\n";
            node_id = i;
            break; 
            } // end if             
        } // end for
    if( node_id < 0)
        {
        std::cerr << "Could not find your local ip address\n";
        return 3;
        }
    
    // crawlers, parsers
    //APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, 768, 384, 0, seed_links); // For depolyment
    //APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, 4, 4, 0, seed_links); // For testing
    APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, 2500, 1500, 0, seed_links); // Playing around with bounds
    
    merc.user_handler( );
    return 0;
    } // end main( )
