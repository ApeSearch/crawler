CXX = g++
CXXFLAGS = -std=c++11 -g

LDFLAGS = -I/usr/local/opt/openssl/include/ -L/usr/local/opt/openssl/lib -lssl -lcrypto
#List of source files for project
FS_SOURCES=Client_Socket.cpp Client_SSLSocket.cpp 

# Generate the names of the file server's object files
FS_OBJS=${FS_SOURCES:.cpp=.o} 

all: Crawler

Crawler: Socket.cpp SSLSocket.cpp Crawler.cpp GetRequest.cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -lz -o Crawler 

.PHONY: clean

clean:
	rm -f *.o Crawler
