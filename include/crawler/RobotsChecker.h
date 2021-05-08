
#pragma once
#ifndef ROBOTS_H_AS
#define ROBOTS_H_AS

#include "../../libraries/AS/include/AS/string.h"
#include "../../libraries/AS/include/AS/vector.h"

// The main idea here is creating a m-tree with sorted entries

// Associates tokens
enum class TypeOfNode
   {
   // If say Directory/*, then subDirEntries should be one empty,
   // and all subsequent nodes are off-limits... ( or allowed depending on the robots.txt)

   // 2)
   // 
   };


class TreeNode
    {
    APESEARCH::vector<APESEARCH::string> subDirEntries;
    APESEARCH::string dirName;
    bool allowed;
    };
