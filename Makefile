CC=g++ -g3 -std=c++17 -Wall -pedantic -Wconversion -Wextra -Wreorder -fno-builtin

SOURCES=$(wildcard *.cpp)
OBJS=${SOURCES:.cpp=.o}

EXECDIR=tests/bin
TESTDIR=tests

all: LinuxGetUrl test

LinuxGetUrl: ${OBJS}
	${CC} -o $@ $^

# LinuxGetSsl:
# 	${CC} -o $@ $^

# LinuxTinyServer:
# 	${CC} -o $@ $^

TEST_SRC:=$(basename $(wildcard ${TESTDIR}/*.cpp))
$(TEST_SRC): %: %.cpp ParsedUrl.o
	${CC} -D LOCAL -o ${EXECDIR}/$(notdir $@) $^ 

test: ${TEST_SRC} 

%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f LinuxGetUrl *.o tests/bin/*
