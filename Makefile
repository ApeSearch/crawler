CC=g++ -g3 -std=c++17 -Wall -pedantic -Wconversion -Wextra -Wreorder -fno-builtin
CXX=g++ -O3 -DNDEBUG -std=c++1z -pedantic -Wconversion -Wextra -Wreorder -fno-builtin
CXXFLAGS = -std=c++1z

# Define certain variables based on system
ifeq ($(shell uname -s | tr A-Z a-z), darwin)
	LDFLAGS:=-lpthread -I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib  -lssl -lcrypto
	# -I likely means (Include) -L likely means (Library)
endif

ifeq ($(shello uname -s | tr A-Z a-z), linux)
	LDFLAGS
endif

SOURCES=$(wildcard *.cpp)
#GETURLSOURCES=$(wildcard GetUrl/*.cpp)
OBJS=${SOURCES:.cpp=.o}
#URLOBJS=${GETURLSOURCES:.cpp=.o}


MODULEDIR=bin
EXECDIR=tests/bin
TESTDIR=tests

all: LinuxGetUrl  test

LinuxGetUrl: ${OBJS}
	${CC} -o $@ $^

LinuxGetSsl: GetUrl/LinuxGetSsl.cpp
	$(CC) $^ $(LDFLAGS) -lz -o ${MODULEDIR}/$(notdir $@)

# LinuxGetSsl:
# 	${CC} -o $@ $^

# LinuxTinyServer:
# 	${CC} -o $@ $^

TEST_SRC:=$(basename $(wildcard ${TESTDIR}/*.cpp))
$(TEST_SRC): %: %.cpp ParsedUrl.o
	${CC} -D LOCAL -o ${EXECDIR}/$(notdir $@)

test: ${TEST_SRC} 

%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

debug: CC += -g3 -DDEBUG
debug: clean all


release: CC=${CXX}
release: clean all

.PHONY: clean

clean:
	rm -f LinuxGetUrl LinuxGetSsl *.o tests/bin/* *.exe bin/*

Christian:
	git config user.name "cluc"
	git config user.email "cluc@umich.edu"