# crawler

The General Algorithm

Set<URL> seen; // Will be implemented as a bloom filter for better space
Queue<URL> frontier = getSeedList();

while(true) {
	URL url = frontier.pop_front();
	HTML content = download(url);
	ParsedHTML parsed = parse(content);

	save parsed.content to be indexed later

	for link in parsed.links which are not in seen:
	add link to frontier
} // end while

# Crawler

This repository houses the crawler, node between machines when passing urls and how to store and process anchor text.

It starts up by running the following script ( after compiling with make clean; make -j tests/crawler ) ./scripts/start.sh " in the top-level directory.

Must run git submodule update --init --remote to download the submodules (or you could drag our libraries repo into the directory).

# Classes

Each of the class header files are located in crawler/include/crawler, and their implementations (if applicable) are in src/.

## SetOfUrl

A class dedicated to writing urls to files and maintaining the front of it (a randomly picked url from the directory) and a backend file for which urls are written to it.

## UrlFrontier

The UrlFrontier is the main method of prioritizing urls and providing some sort of politeness thorugh the backendPolitenessPolicy.

## Node

Provides a writeFile(filename) method, which allows users to create a directory full of Apechunks.  writeFile() adds documents to the index until it reaches a certain number of Posts, then serializes the index into an ApeChunk.

## Database

Database is an object that manages writing parsed files and anchor text into a group of bucket files (1024). This is to make it easy to group together anchor text with its corresponding parsed files.

## Request

This the main requesting object that gets a request, parses the header and decides what to do afterwards.

## SSLSocket

An inherited class from sockets.h (look in libraries for more details).

# Tests

Similar to query_constraint, this repository contains unit tests and a few drivers for different parts of the project. Theses are located in tests where many of the basic functionalities were tested.
