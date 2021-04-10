#include "../include/crawler/SSLSocket.h"
#include "../include/crawler/Node.h"
#include <iostream>
using APESEARCH::unique_ptr;

#define PORT 8080
#define NODECOUNT 8

Node::Node(vector<string> ips, string loc_ip) : sockets(ips.size()), addrinfos(ips.size()), locks(ips.size()), local_ip(loc_ip), ip_addresses(ips)
{
    constexpr char *pathname = "./storage_files/storage_filei.txt"
    //Get handles to files
    for(int i = 0; i < ip_addresses.size(); ++i)
    {
        //TODO decide on pathnames 
        pathname[28] = '0' + i;

        try
            { 
            //Make sure that we never have more than one process appending unto these files.
            storage_files.emplace_back(File(pathname, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600));
            }
        catch(failure &fail)
            {
            //If you fail here we got nothing to do except manual debug
            std::cerr << "Failed opening node storage file " << pathname << '\n';
            //TODO probably need to exit immediatly since you don't have communication storage space
            exit(1);
            }
    }

    //Create addrinfo structures for connecting
    for(int i = 0; i < ip_addresses.size(); ++i)
    {
        struct addrinfo &addr = addrinfos[i];
        memset(&(addr), 0, sizeof(addr));
        addr.sin_family = AF_INET;    /* select internet protocol */
        addr.sin_port = htons(PORT);         /* set the port # */
        addr.sin_addr.s_addr = inet_addr(ips[i].c_str()) /* set the addr */

        if(ips[i] == local_ip)
            node_id = i;
    }


    //Premptive to connect to other nodes that have larger ids
    for(int i = node_id + 1; i < ip_addresses.size(); ++i)
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
        std::cerr << "Error in starting up listening port" << '\n';
    }
    
    //Call thread with ConnectionHandler



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
        
        try
        {
            unique_ptr<Socket> ptr = server.accept((struct sockaddr *) &node_addr, &node_len);
            char *ip = inet_ntoa(node_addr.sin_addr);

            //TODO strcmp vs make a string operator=
            
            for (int i = 0; i < NODECOUNT; ++i)
            {
                if(!strcmp(ip, ips[i].cstr()))
                    {
                    APESEARCH::unique_mutex<APESEARCH::mutex> lk( locks[i] );
                    sockets[i].swap(ptr);
                    }
            }
        }
        catch(...)
        {
            std::cerr << "Failed accepting call\n";
        }
    }

}

//Try to send n times unless the connection is closed,
//Then try to create a new socket
//If sent n times and connection is still open or new connection cannot be made write to swap file
void Node::send( )
{

}

//Constantly reading from sockets
void Node::receive()
{

}