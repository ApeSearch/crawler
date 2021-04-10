#include "../../libraries/AS/include/AS/vector.h"
#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/Socket.h"
#include "../../libraries/AS/include/AS/unique_ptr.h"
#include "../../libraries/AS/include/AS/mutex.h"
#include "../../libraries/AS/include/AS/File.h"
#include "../../Parser/HtmlParser.h"
#include "../../libraries/HashTable/include/HashTable/FNV.h"

class Node
{
private:
    APESEARCH::unique_ptr<Socket> server;
    APESEARCH::vector<struct addrinfo> addrinfos;
    APESEARCH::vector<APESEARCH::unique_ptr<Socket>> sockets;
    APESEARCH::vector<APESEARCH::File> storage_files;
    APESEARCH::vector<APESEARCH::mutex> locks; //Used for sends, and atomic updates to storage_files
    APESEARCH::vector<APESEARCH::string> ips;
    APESEARCH::string local_ip;
    int node_id;
    FNV hash;

    

public:
    //Try to connect to other nodes from ips
    //Start listening server
    //Check if swap files exist and how much data they have in them currently
    //Must have ips in some ordering!
    Node(APESEARCH::vector<APESEARCH::string> &ips, APESEARCH::string &loc_ip);
    ~Node();

    //1 dedicated thread-blocking
    void connectionHandler();

    //On functioncall stack
    //Try to send n times 
    //If cannot send write to local file
    //
    void send(Link &link);

    // 7 dedicated threads non-blocking
    void receive();
};

