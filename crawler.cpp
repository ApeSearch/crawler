
#include "include/crawler/Mercator.h"
#include "include/crawler/SSLSocket.h"
#include "libraries/AS/include/AS/algorithms.h"
#include <signal.h> // For SIGPIPE
#include <fstream>
#include <signal.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h> // For strcmp

// #define MAXNODES 2
const int max_nodes_c = 1;
// For Deployment: 768,384,0
// Stress Testing: 2500,1500,0
// Amount of total hardware threads (on my PC): 4,4,0
const int amt_crawlers_c = 4; 
const int amt_parsers_c = 4; 
const int amt_writers_c = 0; 
//NOTE: The sum should represent the upper bound of threads to be allocated

int main( int argc, char **argv )
    {
    // Initialize SSLlibrary
    SSL_library_init( ); // Initialize relevant resources
    //SIGPIPE setting
    signal(SIGPIPE, SIG_IGN); // Prevents crashing if SIGPIPE signal occurs
    
    if ( argc < 2 ) 
        {
        std::cerr << "Please include host IP address as follows: \n\t/PATH/TO/crawler <IP_Address>"  << std::endl;
        return 1;
        } // end if
    printf("IP address: %s\n", argv[1]);

    APESEARCH::vector<Link> seed_links;
    std::string url;

    std::ifstream in( "./crawling_list.txt" );
    
    // Input starting urls from crawling list into url vector (idempotent so affects the ULR frontier once)
    while( in >> url )
        {
        seed_links.emplace_back( APESEARCH::string(url.begin(), url.end()) );
        } // end while

    
    //Nikolas1, Nikolas2, Robin1, Robin2, Paul1, Paul2, Christian1, Christian2 Serdar1, Serdar2, Alex1, Alex2
    //APESEARCH::vector<APESEARCH::string> ips = { "35.230.41.55", "34.71.229.2", "34.75.57.124", "35.245.134.242", "35.194.60.3", "35.232.126.246", "199.223.236.235", "35.194.73.46", "35.231.170.57", "34.86.240.51", "34.73.221.32", "34.86.225.197" };
    // APESEARCH::vector<APESEARCH::string> ips = { "34.75.57.124", "35.245.134.242" };
    APESEARCH::vector<APESEARCH::string> ips = { "1.1.1.1.1" };
    if(ips.size() != max_nodes_c )
        {
        std::cerr << "Wrong amount of ips: " << ips.size() << std::endl;
        return 2;
        } // end if

    APESEARCH::string* itr = APESEARCH::find_if<>( ips.begin( ), ips.end( ), 
                       [ip{ argv[1] }](  const APESEARCH::string& ip_of ) -> bool { return !strcmp( ip_of.cstr( ), ip ); } 
                      );
    if ( itr == ips.end( ) )
      {
      std::cerr << "Could not find your local ip address\n";
      return 3;
      } // end if
    const int node_id = itr - ips.begin( );
    std::cerr << "Starting to run on this ip: " << *itr <<  " this is Node " << node_id << "\n";

    //int node_id = -1;
    //APESEARCH::string ip( argv[ 1 ] );
    // for( int i = 0; i < ips.size(); ++i )
    //     {
    //     if( ips[ i ] == ip ) 
    //         {
    //         std::cerr << "Starting to run on this ip: " << ips[ i ] <<  " this is Node " << i << "\n";
    //         node_id = i;
    //         break; 
    //         } // end if             
    //     } // end for
    // if( node_id < 0)
    //     {
    //     std::cerr << "Could not find your local ip address\n";
    //     return 3;
    //     }
    //std::cerr << "Starting to run on this ip: " << ips[ i ] <<  " this is Node " << i << "\n";
    
    // crawlers, parsers, writers
    APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, amt_crawlers_c, amt_parsers_c, amt_writers_c, seed_links); // For testing
    
    merc.user_handler( );
    return 0;
    } // end main( )
