CC=g++ -g3 -std=c++17 -Wall -pedantic -Wconversion -Wextra -Wreorder -fno-builtin
CXX=g++ -O3 -DNDEBUG -std=c++17 -pedantic -Wconversion -Wextra -Wreorder -fno-builtin
CXXFLAGS = -std=c++1z

LDFLAGS:=-I/usr/local/opt/openssl/include -L/usr/local/opt/openssl/lib -lssl -lcrypto -lpthread -lz
# Define certain variables based on system
# -I likely means (Include) -L likely means (Library)
FLAGS:=EMPTY
ifeq ($(shell uname -s | tr A-Z a-z), darwin)
	OSFLAGS:=_LARGE_FILE_API
	FLAGS:=MACOS
endif

ifeq ($(shello uname -s | tr A-Z a-z), linux)
	OSFLAGS:=EMPTY
	FLAGS:=LINUX
endif

SRC=src
SOURCES=$(wildcard ${SRC}/*.cpp)
OBJS=${SOURCES:.cpp=.o}

ASSRC=libraries/AS/src
ASSOURCES=$(wildcard ${ASSRC}/*.cpp)
ASOBJS=${ASSOURCES:.cpp=.o}

PARSERSOURCES=$(wildcard Parser/*.cpp)
PARSEROBJS=${PARSERSOURCES:.cpp=.o}


FrameWorkSRC=libraries/unit_test_framework/src
FrameWorkSources=$(wildcard ${FrameWorkSRC}/*.cpp)
FrameWorkOBJS=$(FrameWorkSources:.cpp=.o)

MODULEDIR=bin
EXECDIR=tests/bin
OUTPUT=tests/output
STDEXECDIR=tests/std_bin
TESTDIR=tests


all: test

LinuxGetUrl: ${OBJS}
	${CC} -o $@ $^

LinuxGetSsl: GetUrl/LinuxGetSsl.cpp
	$(CC) $^ $(LDFLAGS) -lz -o ${MODULEDIR}/$(notdir $@)

# LinuxGetSsl:
# 	${CC} -o $@ $^

# LinuxTinyServer:
# 	${CC} -o $@ $^
TEST_SRC:=$(basename $(wildcard ${TESTDIR}/*.cpp))

$(TEST_SRC): %: %.cpp ${ASOBJS} ${OBJS} ${FrameWorkOBJS} ${PARSEROBJS}
	@mkdir -p storageFiles parsedFiles anchorFiles
	@mkdir -p ${EXECDIR}
	@mkdir -p ${STDEXECDIR}
	@mkdir -p ${OUTPUT}
	${CC} -Dtesting -DDEBUG $^ $(LDFLAGS) -o ${EXECDIR}/$(notdir $@)

test: ${TEST_SRC} 

crawler: %: %.cpp ${ASOBJS} ${OBJS} ${PARSEROBJS}
	@mkdir -p storageFiles parsedFiles anchorFiles anchorMapFiles0 anchorMapFiles1 anchorMapFiles2 condensedFiles
	@mkdir -p ${MODULEDIR}
	${CC} -D${FLAGS} $^ ${LDFLAGS} -o ${MODULEDIR}/$(notdir $@)

update:

%.o: %.cpp
	${CC} -D${FLAGS} -DTESTING ${LDFLAGS} -c $< -o $@
%.o: %.cc
	${CC} -c $< -o $@

debug: CC += -g3 -DDEBUG
debug: clean all

HtmlParser: Parser/HtmlParser.cpp Parser/HtmlTags.cpp
	${CC} -DLOCAL -o $@ $^

release: CC=${CXX}
release: clean crawler

.PHONY: clean

clean:
	rm -rf LinuxGetUrl LinuxGetSsl *.o tests/bin/* *.exe ${ASOBJS} ${OBJS} ${EXECDIR}/* ${STDEXECDIR}/* ${OUTPUT}/* ${STDEXECDIR}/*

Christian:
	git config user.name "cluc"
	git config user.email "cluc@umich.edu"