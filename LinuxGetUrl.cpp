#include "ParsedUrl.h"

#include <iostream>

int main( int argc, char**argv)
    {
    if (argc != 2)
        {
        std::cerr << "Usage: ./LinuxGtUrl <URL>" << std::endl;
        exit(1);
        }

    // Parse the URL
    auto url = ParsedUrl(argv[1]);
    if (url.valid_url == false)
        {
        std::cerr << "Invalid URL" << std::endl;
        exit(1);
        }

    // Get the host address
    auto address = Address(url.Host.get(), url.Port.get());
    if (address.valid)
        std::cout << address << std::endl;
    else
        {
        std::cout << "Error resolving hostname: " << url.Host.get();
        if (url.Port)
            std::cout << ":" << url.Port.get();
        std::cout << std::endl;
        }

    // Create a TCP/IP socket.

    // Connect the socket to the host address.

    // Send a GET message.

    // Read from the socket until there's no more data, copying it to stdout.

    // Close the socket and free the address info structure.

    }