#include "../include/crawler/Node.h"
#include "../libraries/AS/include/AS/unique_mmap.h"
#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXTHREADS 25
#define BUFFERSIZE 262144
#define PORT 6666

using APESEARCH::unique_ptr;

NodeBucket::NodeBucket(size_t index, const char *ip) : socket(new Socket()), writer_semaphore(0)
{
    APESEARCH::string pathname = "./storageFiles/storage_file";
    char path[PATH_MAX];
    snprintf( path, sizeof( path ), "%s%d%s", pathname.cstr(), index, ".txt" );
    storage_file = APESEARCH::File( path, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );

    memset(&(addr), 0, sizeof(addr));
    addr.sin_family = AF_INET;    // select internet protocol 
    addr.sin_port = htons(PORT);         // set the port # 
    addr.sin_addr.s_addr = inet_addr(ip); // set the addr

}

Node::Node(APESEARCH::vector<APESEARCH::string> &ips, int id, UrlFrontier& fron, Database &db, Bloomfilter &bf) : frontier( fron ), bloomFilter(bf)
    ,pool( MAXTHREADS, MAXTHREADS ), dataBase( db ), node_id( id )
{
    //THE NODE_ID BUCKET WILL NEVER BE USED FOR ANYTHING, NOT WORTH OPTIMIZATION
    node_buckets.reserve( ips.size() );
    for(int i = 0; i < ips.size(); i++)
        {   
            try
            {
                node_buckets.emplace_back( i, ips[i].cstr() );
            }
            catch(APESEARCH::File::failure &f)
            {
                std::cerr << "Failed on creating storage files\n";
                std::cerr << f.getErrorNumber();
                exit(1);
            }
        }

    //Create server socket on your node_id
    try
    {
        unique_ptr<Socket> ptr(new Socket(PORT));
        node_buckets[node_id].socket.swap(ptr);
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

    //Call threadpool for Senders
    for ( unsigned i = 0; i < ips.size(); ++i )
       {
           if(i != node_id)
            {
                auto sen = [this]( int i )
                { this->Node::sender( i ); };
                pool.submitNoFuture(sen, i);
            }
       }
    //Call threadpool for Receivers
    for ( unsigned i = 0; i < ips.size(); ++i )
       {
           if(i != node_id)
            {
                auto rec = [this]( int i )
                { this->Node::receiver( i ); };
                pool.submitNoFuture(rec, i);
            }
       }
}

Node::~Node(){}


void Node::connect( int index )
{
    while(true)
    {
        std::cerr << "Trying to connect to Node: " << index << '\n';
        {
            try
            {
                unique_ptr<Socket> ptr(new Socket(node_buckets[index].addr));
                node_buckets[index].socket.swap(ptr);
                std::cerr << "Successfully connected to node: " << index << '\n';
                return;
            }
            catch(...)
            {
                std::cerr << "Could not connect to Node: " << index << '\n';
            }
        }
    }
}

//Use socket accept to get new sockets
//Use fine grained locking to switch out sockets.
void Node::connectionHandler()
{
    std::cerr << "Connection handler is running \n";
    //Since accept will block this is not spinning
    while(true)
    {
        struct sockaddr_in node_addr;
        socklen_t node_len = sizeof(node_addr);
        memset(&(node_addr), 0, node_len);
        
        bool found_ip = false;

        try
        {
            std::cerr<< "Before " << node_addr.sin_addr.s_addr << '\n';

            unique_ptr<Socket> ptr = node_buckets[node_id].socket->accept((struct sockaddr *) &node_addr, &node_len);

            std::cerr << "After "<< node_addr.sin_addr.s_addr << '\n';

            for (int i = node_id - 1; 0 <= i; --i)
            {
                if( node_addr.sin_addr.s_addr == node_buckets[i].addr.sin_addr.s_addr )
                    {
                    node_buckets[i].socket_lock.lock();
                    node_buckets[i].socket.swap(ptr);
                    found_ip = true;
                    node_buckets[i].cv.notify_all();
                    node_buckets[i].socket_lock.unlock();
                    std::cerr << "Connected to node: " << i << "\n";
                    break;
                    }
            }
            // If someone with a higher node id tries to connect to me write to std::cer
            // Also checks if someone who is not allowed to connect tries to connect to me
            if(!found_ip)
            {
                std::cerr << "Connection to unknown ip cancelled, ip \n";
            }
        }
        catch(...)
        {
            std::cerr << "Failed accepting call on server socket\n";
        }
    }

}

void Node::sender(int index)
{
    //std::cerr << "Sender is running to send on Node: " << index << '\n';
    node_buckets[index].socket_lock.lock();
    int fd = node_buckets[index].socket->getFD();
    node_buckets[index].socket_lock.unlock();
    
    while(true)
    {
        sleep(5u);

        node_buckets[index].writer_semaphore.down();
        //std::cerr << "Sender activated to send to Node: " << index << '\n';
        APESEARCH::vector<char> local;
        {
            node_buckets[index].high_prio_lock();
            ssize_t file_size = node_buckets[index].storage_file.fileSize();
            local.resize(file_size);

            unique_mmap mappedFile;
            try
            {
                mappedFile = unique_mmap( file_size, PROT_READ, MAP_SHARED, node_buckets[index].storage_file.getFD(), 0 );
            }
            catch( unique_mmap::failure& error )
            {
                node_buckets[index].high_prio_unlock();
                std::cerr << "VERY BAD ERROR ALERT NIKOLA unique_mmap failed on file: " << index << '\n';
                std::cerr << error.what( ) << std::endl;
                exit(1);
            }
            //Copy locally
            char *sfile =  reinterpret_cast <char *>(mappedFile.get());
            
            for(ssize_t i = 0; i < file_size; ++i)
            {
                local[i] = sfile[i];
            }
            //Truncate it 
            if( 0 > ftruncate(node_buckets[index].storage_file.getFD(), 0))
            {
                std::cerr << "VERY BAD ERROR ALERT NIKOLA truncating: " << index << '\n';
                exit(1);
            }
            //Unlock
            node_buckets[index].high_prio_unlock();
        }

        //Send call 
        if( 0 > ::send(fd, local.begin(), local.size(), 0))
        {
            //Send fail

            //rewrite the local buffer back in
            node_buckets[index].high_prio_lock();
            node_buckets[index].storage_file.write(local.begin(), local.size());
            node_buckets[index].high_prio_unlock();
            //Try to connect
            fd = retriesConnectAfterFailure(fd, index);
            assert( fd > 0 );
            // Allow to try again
            node_buckets[index].writer_semaphore.up();
        }
        else
        {
            //Send succeeded
            //std::cerr << "SENT BUFFER\n";
            //Reseting semaphore to 0
            std::size_t count = node_buckets[index].writer_semaphore.getCount( );
            node_buckets[index].writer_semaphore.down( count );
        }
    }
}

//Try to send n times unless the connection is closed,
//Then try to create a new socket
//If sent n times and connection is still open or new connection cannot be made write to swap file
void Node::write( Link &link )
{
    static const char* const null_char = "\0";
    static const char* const newline_char = "\n";
    static const char* const space_char = " ";
    size_t val = hash(link.URL.cstr());
    val = val % node_buckets.size();
    bool new_link = false;
    //std::cerr << "Going into writer\n";
    {
        if(!bloomFilter.contains(link.URL))
        {
            new_link = true;
            bloomFilter.insert( link.URL );
        } // end if
        else if(link.anchorText.empty() || link.URL.empty()) {
            return;
        } // end else
    }
    if(val == node_id)
    {
        //std::cerr << "Hashed and writing locally\n";
        dataBase.addAnchorFile(link);
        if(new_link && !link.URL.empty())
        {
            //std::cerr << "WROTE A URL TO THE FRONTIER" << link.URL << "\n";
            frontier.insertNewUrl( std::move( link.URL ) );
            
            //set.enqueue(std::move( link.URL ));
        }
    }
    else{ // Write to storage file and eventually send
        
        node_buckets[val].low_prio_lock();
        //std::cerr << "Writing to storage file\n";
        
        node_buckets[val].storage_file.write(link.URL.cstr(), link.URL.size() + 1 );
        for (size_t i = 0; i < link.anchorText.size(); i++)
        {
            size_t size = link.anchorText[i].size( );
            if( i == link.anchorText.size( ) - 1 )
                ++size;
            else
                link.anchorText[i].push_back( ' ' );
            node_buckets[val].storage_file.write(link.anchorText[i].cstr( ), size );       
        }
        node_buckets[val].writer_semaphore.up();
        node_buckets[val].low_prio_unlock();
    }
}

int Node::retriesConnectAfterFailure( int fd, int index )
    {
    APESEARCH::unique_lock<APESEARCH::mutex> lck(node_buckets[index].socket_lock);
            
    //Someone else changed it beforehand failsafe for recieve
    int checkFD =  node_buckets[index].socket->getFD();
    if(fd != checkFD || checkFD != -1)
        return checkFD;
                
    //Declare it invalid
    unique_ptr<Socket> ptr(new Socket());
    node_buckets[index].socket.swap(ptr);

    if(node_id < index)
    {   
        //std::cerr << "Reciever is connecting to Node: " << index << '\n';
        // We must connect to this socket
        connect(index);
    }
    else
    {
        //std::cerr << "Reciever is waiting for connection to Node: " << index << '\n';
        //Wait for connector to notify you
        node_buckets[index].cv.wait(lck, [this, index]() -> bool { return node_buckets[index].socket->getFD() > 0; } );
        //std::cerr << "Reciever woke up from connection to Node: " << index << '\n';
    }
    return node_buckets[index].socket->getFD();
    } // end retriesConnectAfterFailure( )


//url.com\nword1 word2 word3 \0
//Constantly reading from sockets
void Node::receiver(int index)
{
    //std::cerr << "Reciever is running to rec on Node: " << index << '\n';
    char buffer[BUFFERSIZE];
    APESEARCH::vector< char > intermediateBuf; 
    node_buckets[index].socket_lock.lock();
    int fd = node_buckets[index].socket->getFD();
    node_buckets[index].socket_lock.unlock();
    Link linkOf;
    APESEARCH::string anchor_word = "";
    int bytesRead = 0;
    bool is_url = true;
    
    //[url\n\0]
    //[url\nword \0]
    while( true )
    {
        char *buffPtr, *buffEnd;
        bytesRead = recv(fd, buffer, BUFFERSIZE, 0);
        buffEnd = ( buffPtr = buffer ) +  bytesRead;

        if ( bytesRead == -1 )
            {
            fd = retriesConnectAfterFailure( fd, index );
            assert( fd > 0 );
            intermediateBuf = APESEARCH::vector< char >( );
            continue; 
            } // end if

        for ( ;buffPtr != buffEnd; ++buffPtr )
        {
        switch( *buffPtr )
            {
            // Signifies end of url
            case '\n':
                linkOf.URL = APESEARCH::string( intermediateBuf.begin( ), intermediateBuf.end( ) );
                //TODO change to .clear( );
                intermediateBuf = APESEARCH::vector< char >( );
                break;
            case ' ':
                linkOf.anchorText.emplace_back( intermediateBuf.begin( ), intermediateBuf.end( ) );
                //TODO change to .clear( );
                intermediateBuf = APESEARCH::vector< char >( );
                break;
            case '\0':
                {
                if ( !intermediateBuf.empty( ) )
                    {
                    linkOf.anchorText.emplace_back( intermediateBuf.begin( ), intermediateBuf.end( ) );
                    //TODO change to .clear( );
                    intermediateBuf = APESEARCH::vector< char >( );
                    } // end if
                
               // std::cerr << "RECEIVED BUFFER: "<<  linkOf.URL << "\n";
                // Check bloomfilter
                if( linkOf.URL.empty() )
                    {
                        linkOf = Link();
                        continue;
                    }

                if ( !bloomFilter.contains( linkOf.URL ) )
                    {
                    bloomFilter.insert( linkOf.URL );
                    frontier.insertNewUrl( std::move( linkOf.URL ) );
                    //set.enqueue(std::move( linkOf.URL ));
                    }
                // Add to DB
                dataBase.addAnchorFile(linkOf);

                // Reset Link
                linkOf = Link();
                }
                break;
            default:
                intermediateBuf.push_back( *buffPtr );
            } // end switch
        }//end for
    }  // end while
}
