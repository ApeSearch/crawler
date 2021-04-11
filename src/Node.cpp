#include "../include/crawler/Node.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define FILESIZE_CHECKPOINT 65536
#define MAXTHREADS 15
#define BLOOMFILTER

using APESEARCH::unique_ptr;

#define PORT 8080

Node::Node(APESEARCH::vector<APESEARCH::string> &ips, APESEARCH::string &loc_ip, SetOfUrls& _set, Database &db) : 
    sockets(ips.size()), addrinfos(ips.size()), locks(ips.size()), local_ip(loc_ip), set( _set ), bloomFilter()
    ,pool( Node::CircBuf( MAXTHREADS ), MAXTHREADS, MAXTHREADS ), dataBase(db)
{
    APESEARCH::string pathname = "./storage_files/storage_file";
    char path[PATH_MAX];
    //Get handles to files
    for(int i = 0; i < ips.size(); ++i)
    {
        //TODO decide on pathnames 
        snprintf( path, sizeof( path ), "%s%d%s", pathname.cstr(), i, ".txt" );
        try
            { 
            //Make sure that we never have more than one process appending unto these files.
            storage_files.emplace_back( path, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
            }
        catch(APESEARCH::File::failure &fail)
            {
            //If you fail here we got nothing to do except manual debug
            std::cerr << "Failed opening node storage file " << pathname << '\n';
            //TODO probably need to exit immediatly since you don't have communication storage space
            exit(1);
            }
    }


    
    for(int i = 0; i < ips.size(); ++i)
    {
        struct sockaddr_in &addr = addrinfos[i];
        memset(&(addr), 0, sizeof(addr));
        addr.sin_family = AF_INET;    // select internet protocol 
        addr.sin_port = htons(PORT);         // set the port # 
        addr.sin_addr.s_addr = inet_addr(ips[i].cstr()); // set the addr

        if(ips[i] == local_ip)
            node_id = i;
    }


    //Premptive to connect to other nodes that have larger ids
    for(int i = node_id + 1; i < ips.size(); ++i)
        {
            try
                {
                unique_ptr<Socket> ptr(new Socket(addrinfos[i]));
                sockets[i].swap(ptr);
                }
            catch(...)
            {
                std::cerr << "Could not connect to: " << ips[i] << '\n';
            }
        }

    //Create server socket on your node_id
    try
    {
        unique_ptr<Socket> ptr(new Socket(PORT));
        sockets[node_id].swap(ptr);
    } 
    catch(...)
    {
        std::cerr << "Error in starting up listening socket" << '\n';
        exit(1);
    }
    
    //Call 1 thread with ConnectionHandler
    auto connHandler = [this]( )
       {
        this->connectionHandler();
       };
    pool.submitNoFuture( connHandler );

    //Call threadpool for 7 threads Connectors
    for ( unsigned n = 0; n < ips.size() - 1; ++n )
       {
        std::cout << "lol" << std::endl;
       } // end forvoid addAnchorFile(Link &link);

}

void Node::connector( int i )
{
    while(true)
    {
        {
            APESEARCH::unique_lock<APESEARCH::mutex> lk( locks[i] );

            if(sockets[i]->getFD() > 0)
                return;

            try
            {
                unique_ptr<Socket> ptr(new Socket(addrinfos[i]));
                sockets[i].swap(ptr);
                return;
            }
            catch(...)
            {
                std::cerr << "Could not connect to Node: " << i << '\n';
            }
        }
        sleep( 30u );
    }
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
            unique_ptr<Socket> ptr = sockets[node_id]->accept((struct sockaddr *) &node_addr, &node_len);
            char *ip; //= inet_ntoa(node_addr.sin_addr);

            //TODO strcmp vs make a string operator=

            for (int i = node_id - 1; 0 <= i; --i)
            {
                if( node_addr.sin_addr.s_addr == addrinfos[i].sin_addr.s_addr )
                    {
                    APESEARCH::unique_lock<APESEARCH::mutex> lk( locks[i] );
                    sockets[i].swap(ptr);
                    
                    auto func = [this]( int i )
                        { this->Node::receive( i ); };
                    pool.submitNoFuture(func, i);
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
    static const char* const space_char = " ";
    //TODO switch to modulo operation after testing, since we will have 8 nodes.
    size_t val = hash(link.URL.cstr());
    val = val % addrinfos.size();
    bool new_link = false;
{
    APESEARCH::unique_lock<APESEARCH::mutex> uniqLk( bloomFilter_lock );
    if(!bloomFilter.contains(link.URL))
       {
        new_link = true;
        bloomFilter.insert( link.URL );
       } // end if
    else if(link.anchorText.empty()) {
        return;
    } // end else
}
    if(val == node_id)
    {
        //TODO
        //if not in bloom filter
        if(new_link)
        {
            //  TODO write to frontier
        }

        // TODO Write(anchor text vector) to database
        dataBase.addAnchorFile(link);
        
    }
    else{
        
        APESEARCH::unique_lock<APESEARCH::mutex> lk( locks[val] );

        storage_files[val].write(link.URL.cstr(), link.URL.size());
        storage_files[val].write(newline_char, 1);
        for (size_t i = 0; i < link.anchorText.size(); i++)
        {
            storage_files[val].write(link.anchorText[i].cstr(), link.anchorText[i].size());
            storage_files[val].write(space_char, 1);            
        }
        storage_files[val].write(null_char, 1);
        
        
        ssize_t file_size = lseek(storage_files[val].getFD(), 0, SEEK_END); //find the size
        lseek(storage_files[val].getFD(), 0, SEEK_SET); //Reset

        if(file_size > FILESIZE_CHECKPOINT && sockets[val]->getFD() > 0 )
        {
            try
            {
                unique_mmap mappedFile( file_size, PROT_READ, MAP_SHARED, storage_files[val].getFD(), 0 );
                sockets[val]->send( (const char *) mappedFile.get(), file_size);
                lseek(storage_files[val].getFD(), 0, SEEK_SET); //Reset
                storage_files[val].truncate(0);
            }
            catch(...)
            {
                unique_ptr<Socket> ptr(new Socket());
                ptr.swap(sockets[val]);
                if(val > node_id)
                    {
                    auto func = [this]( int val )
                    { this->connector( val ); };
                    pool.submitNoFuture(func, val );
                    }
            }
            
        }
         
    }
}

//Constantly reading from sockets
void Node::receive(int i)
{
    int fd;
    {
        APESEARCH::unique_lock<APESEARCH::mutex> lk( locks[i] );
        fd = sockets[i]->getFD();
    }

    char buffer[FILESIZE_CHECKPOINT];
    Link linkOf;
    size_t bytesRead;
    bool is_url = true;
    while(true)
    {
        bytesRead = recv(fd, buffer, FILESIZE_CHECKPOINT, 0);

        for (size_t i = 0; i < bytesRead; ++i)
        {
            if(buffer[i] == '\n')
            {
                is_url = false;
            }
            else if(buffer[i] == '\0')
            {
                bool found = false;
                is_url = true;
                {
                    APESEARCH::unique_lock<APESEARCH::mutex> lk( bloomFilter_lock );                   
                    found = bloomFilter.contains(linkOf.URL);
                }
                if(!found)
                {
                    //TODO Write to Frontier
                }
                    
                dataBase.addAnchorFile(linkOf);
                
                //Push link
            }
            else if(buffer[i] == ' ')
            {
                linkOf.anchorText.push_back("");
            }
            else
            {
                if(is_url)
                {
                    linkOf.URL.push_back(buffer[i]);
                }
                else
                {
                    linkOf.anchorText.back().push_back(buffer[i]);
                }
            }
        }
        if (bytesRead == -1)
        {
            APESEARCH::unique_lock<APESEARCH::mutex> lk( locks[i] );
            if(fd == sockets[i]->getFD())
            {
                unique_ptr<Socket> ptr(new Socket());
                ptr.swap(sockets[i]);
                //Only connect if he has a smaller number than you
                if(node_id < i)
                {
                    auto func = [this]( int i )
                    { this->connector( i ); };
                    pool.submitNoFuture(func, i );
                }
            }
            break;
        }
    }  
    //Call recieve on socket
    //Parse data into url section and 
}