#include "../libraries/AS/include/AS/vector.h"
#include "../libraries/AS/include/AS/string.h"
#include "../libraries/AS/include/AS/Socket.h"
#include "../libraries/AS/include/AS/unique_ptr.h"
#include "../libraries/AS/include/AS/mutex.h"
#include "../libraries/AS/include/AS/File.h"
#include "../../Parser/HtmlParser.h"
#include "../libraries/AS/include/AS/FNV.h"

class Node
{
private:
    unique_ptr<Socket> server;
    vector<struct addrinfo> addrinfos; 
    vector<unique_ptr<Socket>> sockets;
    vector<File> storage_files;
    vector<mutex> locks; //Used for sends, and atomic updates to storage_files
    string local_ip;
    int node_id;
    FNV hash;

    

public:
    //Try to connect to other nodes from ips
    //Start listening server
    //Check if swap files exist and how much data they have in them currently
    //Must have ips in some ordering!
    Node(vector<string> &ips, string &loc_ip);
    ~Node();

    //1 dedicated thread-blocking
    connectionHandler();

    //On functioncall stack
    //Try to send n times 
    //If cannot send write to local file
    //
    send(Link &link);

    // 7 dedicated threads non-blocking
    receive();
};

