
#pragma once

#ifndef DOCPROCESSOR_H_APESEARCH
#define DOCPROCESSOR_H_APESEARCH
#include <string>

class DocumentProcessor
{
    char * urlsToCrawl_back; // memory-mapped back of queue of urls to be crawled
    char * indexBuilder_front;

    //Writes HTML to file
    void writeHTMLToFile();


    // Write object to fs
    // Write into queue for index builder
    void writeDocumentObjectToFile();

    //Returns string of size 65536 unless buffer provided is the same size;
    //Some type of buffer resizing to handoff to Crawler
    std::string revertString();


};

#endif