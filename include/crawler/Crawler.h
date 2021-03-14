
#pragma once

#ifndef CRAWLER_H_APESEARCH
#define CRAWLER_H_APESEARCH


#include "Request.h"
#include "Frontier.h"
#include "DuplicateUrlElim.h"
#include "../libraries/AS/include/AS/vector.h"
#include "../Parser/HtmlParser.h"

class Crawler
{
    UrlFrontier *frontier; //Shared frontier across many 
    // DUE
    // Parser
    Request requester;
};


