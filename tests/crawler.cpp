
#include "../include/crawler/Mercator.h"
#include <fstream>
#include <signal.h>

#define MAXNODES 4
// ./exec <NODE_ID> 
// ./exec 1 < seed_list.txt
//34.201.187.203 ( 1 )
int main( int argc, char **argv )
    {
    
    int node_id = atoi( argv[ 1 ] );
    if ( node_id < 0 || node_id > MAXNODES  )
       {
       std::cerr << "Node id is out of bounds: " << node_id << std::endl;
       return 1;
       } // end if
    
    APESEARCH::vector<Link> seed_links;
    std::string url;

    std::ifstream in( "./crawling_list.txt" );
    
    while( in >> url )
        {
        Link link;
        link.URL = APESEARCH::string(url.begin(), url.end());
        seed_links.push_back( link );
        } // end while

    signal(SIGPIPE, SIG_IGN); // Ignores SIGPIPE (tried to write to a socket that closed)
    //Christians,Alexs,Pauls_first, Pauls_second
    APESEARCH::vector<APESEARCH::string> ips = {"54.84.17.85","34.201.187.203","23.21.84.212","34.233.155.58"};
    if(ips.size() != 4)
        {
        std::cerr << "Wrong amount of ips: " << ips.size() << std::endl;
        return 2;
        } // end if
    
    
    // crawlers, parsers, writers
    APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, 1024, 512, 0, seed_links);
    
    merc.user_handler( );
    return 0;
    } // end main( )
