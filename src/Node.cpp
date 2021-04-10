#include "../include/crawler/Node.h"
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>


using APESEARCH::unique_ptr;

#define PORT 8080

Node::Node(APESEARCH::vector<APESEARCH::string> ips, APESEARCH::string loc_ip) : sockets(ips.size()), addrinfos(ips.size()), locks(ips.size()), local_ip(loc_ip)
{
    constexpr char *pathname = "./storage_files/storage_filei.txt";
    //Get handles to files
    for(int i = 0; i < ips.size(); ++i)
    {
        //TODO decide on pathnames 
        pathname[28] = '0' + i;

        try
            { 
            //Make sure that we never have more than one process appending unto these files.
            storage_files.emplace_back( pathname, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
            }
        catch(APESEARCH::File::failure &fail)
            {
            //If you fail here we got nothing to do except manual debug
            std::cerr << "Failed opening node storage file " << pathname << '\n';
            //TODO probably need to exit immediatly since you don't have communication storage space
            exit(1);
            }
    }

    //Create addrinfo structures for connecting
    for(int i = 0; i < ips.size(); ++i)
    {
        struct sockaddr_in &addr = addrinfos[i];
        memset(&(addr), 0, sizeof(addr));
        addr.sin_family = AF_INET;    /* select internet protocol */
        addr.sin_port = htons(PORT);         /* set the port # */
        addr.sin_addr.s_addr = inet_addr(ips[i].cstr()) /* set the addr */

        if(ips[i] == local_ip)
            node_id = i;
    }


    //Premptive to connect to other nodes that have larger ids
    for(int i = node_id + 1; i < ips.size(); ++i)
        {
        try
            {
            Socket *sock = new Socket(addrinfos[i]);
            unique_ptr<Socket> ptr(sock);
            sockets[i].swap(ptr);
            }
        //TODO make custom catch
        catch(...)
            {
            std::cerr << "Failed connecting to ip: " << ips[i] << '\n';
            }
        }

    //Create server socket
    try
    {
        server = unique_ptr<Socket>(new Socket(PORT));
    } 
    catch(...)
    {
        std::cerr << "Error in starting up listening socket" << '\n';
        exit(1);
    }
    
    //Call thread with ConnectionHandler

    //Call threadpool for Connectors

    //Create threadpool for calling recieve()




}

//RAII Theoretically should never be called
Node::~Node()
{

}

//Use socket accept to get new sockets
//Use fine grained locking to switch out sockets.
void Node::connectionHandler()
{
    //Since accept will block this is not spinning
    while(true)
    {
        struct sockaddr_in node_addr;
        socklen_t node_len = sizeof(node_addr);
        
        bool found_ip = false;

        try
        {
            unique_ptr<Socket> ptr = server->accept((struct sockaddr *) &node_addr, &node_len);
            char *ip = inet_ntoa(node_addr.sin_addr);

            //TODO strcmp vs make a string operator=

            for (int i = node_id - 1; 0 <= i; --i)
            {
                if(!strcmp(ip, ips[i].cstr()))
                    {
                    APESEARCH::unique_lock<APESEARCH::mutex> lk( locks[i] );
                    sockets[i].swap(ptr);
                    found_ip = true;
                    break;
                }
            }
            // If someone with a higher node id tries to connect to me write to std::cer
            // Also checks if someone who is not allowed to connect tries to connect to me
            if(!found_ip)
            {
                std::cerr << "Connection to unknown ip cancelled, ip: " << ip << '\n';
            }
        }
        catch(...)
        {
            std::cerr << "Failed accepting call on server socket\n";
        }
    }

}

//Try to send n times unless the connection is closed,
//Then try to create a new socket
//If sent n times and connection is still open or new connection cannot be made write to swap file
void Node::send( Link &link )
{
    static const char* const null_char = "\0";
    static const char* const newline_char = "\n";

    //TODO switch to modulo operation after testing, since we will have 8 nodes.
    size_t val = hash(link.URL.cstr());
    val = val % addrinfos.size();

    if(val == node_id)
    {
        //if not in bloom filter
        //  write to frontier

        // Write(anchor text vector) 
    }
    else
    {
        
        if( sockets[val]->fd() > 0 )
        {
            try
            {
                
            }
            catch(...)
            {
                
            }
            
        //  Call send
        //If send doesn't work retry up to 3 times

        }   
        
        //If it doesn't work swap the Socket with -1 fd socket
        //  Append buffer to appropriate file
        //  Maybe some type of signal to connector?
    }
}

//Constantly reading from sockets
void Node::receive()
{
    //Call recieve on socket
    //  Parse data into url section and 
}