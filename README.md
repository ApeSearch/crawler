# crawler


The General Algorithm


Set<URL> seen; // Will be implemented as a bloom filter for better space
Queue<URL> frontier = getSeedList();

while(true) {
	URL url = frontier.pop_front();
	HTML content = download(url); // similar to SSL lab...
	ParsedHTML parsed = parse(html);
	
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

The Index class (located at the very bottom of Index.h) takes in a directory of ApeChunks (serialized index chunk files), memory maps them as an IndexBlob (a HashBlob), and allows it to be accessed as an in-memory hash table whose keys are words and values are posting lists of variable-length encoded deltas. 

The different ListIterators provide a way for users to give the iterator an absolute location relative to the beginning of the index, and get back the first Post after that location. The ListIterator takes care of decoding the variable-length encoded deltas back into locations.

## Node

Provides a writeFile(filename) method, which allows users to create a directory full of Apechunks.  writeFile() adds documents to the index until it reaches a certain number of Posts, then serializes the index into an ApeChunk.

## Database

IndexHT provides the ability for a user to add documents into an in-memory Inverse Index. The inverse index contains posting lists that map from words to their location relative to the beginning of the index.

## Request

This is the constraint solver, which uses Index Stream Readers (ISRs) to search through each ApeChunk. Provides the ability to seek to the first location after a target that matches a structured ISR tree.

## SSLSocket

Provides the ability for a user to construct a structured parse tree from a search query. Changes search words into an OR expression with decorated words underneath, and phrases into an OR expression with decorated phrases underneath.

## Ranker

Takes in a search query, an ApeChunk (synonymous with IndexBlob, they're basically the same thing) pointer, and provides the ability to construct the most relevant matches based on mostly dynamic ranking factors.

Ranks based on number of in-order and short spans, spans near the top, and exact phrase matches. Also finds matches to the query in Anchor Text and in the URL/Title.

# Tests
Similar to query_constraint, this repository contains unit tests and a few drivers for different parts of the project. Theses are located in tests where many of the basic functionalities were tested.
