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

## Componenets

# Query-Constraint

This repository houses most of the backend code for serving queries to users. 

To see a small example, run "./tests/APESEARCH/apesearch" from the query-constraint top-level directory.

Must run git submodule update --init --remote to download the submodules (or you could drag our libraries repo into the directory).

# Classes

Each of the class header files are located in query-constrain/include, and their implementations (if applicable) are in query-constraint

## Builder

Builder provides the ability to build an in-memory index from a list of processed Apefiles which contain a URL, words in the document corresponding to the URL, and other useful information. 

## Index

The Index class (located at the very bottom of Index.h) takes in a directory of ApeChunks (serialized index chunk files), memory maps them as an IndexBlob (a HashBlob), and allows it to be accessed as an in-memory hash table whose keys are words and values are posting lists of variable-length encoded deltas. 

The different ListIterators provide a way for users to give the iterator an absolute location relative to the beginning of the index, and get back the first Post after that location. The ListIterator takes care of decoding the variable-length encoded deltas back into locations.

## IndexFileParser

Provides a writeFile(filename) method, which allows users to create a directory full of Apechunks.  writeFile() adds documents to the index until it reaches a certain number of Posts, then serializes the index into an ApeChunk.

## IndexHT

IndexHT provides the ability for a user to add documents into an in-memory Inverse Index. The inverse index contains posting lists that map from words to their location relative to the beginning of the index.

## ISR

This is the constraint solver, which uses Index Stream Readers (ISRs) to search through each ApeChunk. Provides the ability to seek to the first location after a target that matches a structured ISR tree.

## QueryParser

Provides the ability for a user to construct a structured parse tree from a search query. Changes search words into an OR expression with decorated words underneath, and phrases into an OR expression with decorated phrases underneath.

## Ranker

Takes in a search query, an ApeChunk (synonymous with IndexBlob, they're basically the same thing) pointer, and provides the ability to construct the most relevant matches based on mostly dynamic ranking factors.

Ranks based on number of in-order and short spans, spans near the top, and exact phrase matches. Also finds matches to the query in Anchor Text and in the URL/Title.

## Token/TokenStream

Provides a stream of Token objects corresponding to different tokens in a search query. TokenStream provides the ability to get the next token and view the current token. 

Tokens include:
AND operators: &&, &, and AND
OR operators: ||, |, and OR
NESTED operators: ( and )
PHRASE operator: "
NOT operators: NOT and -

## Tuple/TupleList

Provides the ability to construct an ISR tree from a Parse tree by calling compile() on the top-level expression in the parse tree.

# Tests
Contains unit tests and a few drivers for different parts of the project. token_tool.cpp (make tests/token_tool) allows you to type in a search query and it prints out the tokens from the token stream. apesearch.cpp (make tests/apesearch) allows you to type in a search query and find matching documents in a directory of ApeChunks.
