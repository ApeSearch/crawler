
#include "include/crawler/Mercator.h"
#include <fstream>

#define MAXNODES 8
// ./exec <NODE_ID> < IP ADDRESSES >
// ./exec 1 < IPaddress.txt
int main( int argc, char **argv )
    {
    
    int node_id = atoi( argv[ 1 ] );
    if ( node_id < 0 || node_id > MAXNODES  )
       {
       std::cerr << "Node id is out of bounds: " << node_id << std::endl;
       return 1;
       } // end if
    
    
    APESEARCH::vector<APESEARCH::string> ips;
    std::string ip;
    //while( std::cin >> ip )
        {
        // ips.emplace_back( ip.begin( ), ip.end( ) );
        } // end while

    ips = {"1","1","1","1","1","1","1","1"};
    if(ips.size() != 8)
        {
        std::cerr << "Wrong amount of ips: " << ip.size() << std::endl;
        return 2;
        } // end if
    


    
    APESEARCH::Mercator merc(ips, node_id, nullptr, nullptr, 10, 10, 0);
    
    merc.user_handler( );
    return 3;
    } // end main( )