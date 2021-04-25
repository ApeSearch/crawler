#include "../include/crawler/Database.h"
#include <stdlib.h>
#include <iostream>
#include <unistd.h> // for getcwd



//#include <linux/limits.h>

#define NUM_OF_FILES 1024
#define MAX_PATH 1024
#define MAX_INT_LENGTH 100


void formatFile(APESEARCH::File &file){
    if(file.fileSize() == 0){
        return;
    }
    unique_mmap map( file.fileSize(), PROT_READ, MAP_SHARED, file.getFD(), 0 );
    char const *ptr = reinterpret_cast< char const *>( map.get() ); 
    int end = file.fileSize() - 1;
    while(end >= 0 && ptr[end] != '\0'){
        end--;
    }
    file.truncate(end + 1);

}

bool sortbysecdesc(const std::pair<std::string,int> &a,
                   const std::pair<std::string,int> &b)
{
       return a.second > b.second;
}

void reduceFile( const std::string& path )
    {
    APESEARCH::File file( path.c_str(), O_RDWR, (mode_t) 0600 );
    if(file.fileSize() == 0){
        return;
    }
    unique_mmap mmap( file.fileSize(), PROT_READ, MAP_SHARED, file.getFD(), 0 );
    std::unordered_map<std::string, int> phraseFreq;
    char const *mmapPtr = reinterpret_cast< char const *>( mmap.get() );
    char const *it = mmapPtr;
    while(it < mmapPtr + file.fileSize()){
        std::string phrase = "";
        while(*it != '\n'){
            phrase += *it;
            it++;
        }
        assert(*it == '\n');
        it++;
        if(phraseFreq.find(phrase) == phraseFreq.end()){
            phraseFreq[phrase] = 0;
        }
        phraseFreq[phrase]++;
    }
    file.truncate(0);
    APESEARCH::vector<std::pair<std::string, int>> tempVec;
    for( auto& i : phraseFreq ) {
        tempVec.push_back(std::make_pair(i.first, i.second));
    }
    std::sort(tempVec.begin(), tempVec.end(), sortbysecdesc);
    std::string writeString = "";
    for(int i = 0; i < tempVec.size(); i++){
        tempVec[i].first = "\"" + tempVec[i].first + " ";
        tempVec[i].first[tempVec[i].first.length() - 2] = '\"';
        writeString += tempVec[i].first;
        writeString += std::to_string(tempVec[i].second);
        writeString += '\n';
    }
    file.write(writeString.c_str(), writeString.length());
}



void fillVectors(APESEARCH::vector<size_t> &titleWords, 
APESEARCH::vector<size_t> &headingWords, APESEARCH::vector<size_t> &boldWords, const HtmlParser &parsed){
    
    for(size_t i = 0; i < parsed.parsed_text.size(); i++){
        if(parsed.parsed_text[i].type == titleWord){
            titleWords.push_back(i);
        }
        else if(parsed.parsed_text[i].type == headingWord){
            headingWords.push_back(i);
        }
        else if(parsed.parsed_text[i].type == boldWord){
            boldWords.push_back(i);
        }
    }
}

void writeIndex(APESEARCH::vector<size_t>& indexes, APESEARCH::File &file){
    static const char* const newline_char = "\n";
    static const char* const space_char = " ";
    char temp[MAX_INT_LENGTH];
    for( int i = 0; i < indexes.size(); i++ )
    {
        size_t size = snprintf( temp, sizeof( temp ), "%d", indexes[i]);
        file.write(temp, size);
        file.write(space_char, 1);
    }
    file.write(newline_char, 1);
}

DBBucket::DBBucket( ) : DBBucket( 0, nullptr ) { }

DBBucket::DBBucket(size_t index, const char * dir ){
    if ( !dir )
        snprintf( directory, sizeof( directory ), "%s", "." );
    else
        snprintf( directory, sizeof( this->directory ), "%s", dir );
    static const APESEARCH::string anchor_root = "/anchorFiles/anchorFile";
    static const APESEARCH::string parsed_root = "/parsedFiles/parsedFile";

    char path[1024];
    snprintf( path, sizeof( path ), "%s%s%d%s", directory, anchor_root.cstr(), index, ".txt" );
    anchorFile = APESEARCH::File( path, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
    snprintf( path, sizeof( path ), "%s%s%d%s", directory, parsed_root.cstr(), index, ".txt" );
    parsedFile = APESEARCH::File( path, O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );

    formatFile(anchorFile);
    formatFile(parsedFile);
}

Database::Database( ) : Database( nullptr ) { }

Database::Database( const char *directory )
{
    char path[MAX_PATH];
    file_vector.reserve( NUM_OF_FILES );
    for(int i = 0; i < NUM_OF_FILES; i++)
    {   
        try
        {
            file_vector.emplace_back( i, directory );
        }
        catch(APESEARCH::File::failure &f)
        {
            std::cerr << f.getErrorNumber();
            exit(1);
        }
    }
}
Database::~Database(){}


void Database::addAnchorFile(const Link &link ){
    if(link.anchorText.empty())
    {
        return;
    }
    static const char* const null_char = "\0";
    static const char* const newline_char = "\n";
    static const char* const space_char = " ";

    size_t index = hash(link.URL.cstr());
    index = index % file_vector.size();

    {
        APESEARCH::unique_lock<APESEARCH::mutex> lk( file_vector[index].anchor_lock );
        file_vector[index].anchorFile.write(link.URL.cstr(), link.URL.size());
        file_vector[index].anchorFile.write(newline_char, 1);
        for(size_t i = 0; i < link.anchorText.size(); i++){
            file_vector[index].anchorFile.write(link.anchorText[i].cstr(), link.anchorText[i].size());
            file_vector[index].anchorFile.write(space_char, 1);
        }
        file_vector[index].anchorFile.write(newline_char, 1);
        file_vector[index].anchorFile.write(null_char, 1);
    }
}

void Database::addParsedFile( const HtmlParser &parsed )        //url, parsed text, base, num paragraphs, num headings, num sentences
{                                                       //space delimited between words, newline delimited between sections, null character at end
    static const char* const null_char = "\0";
    static const char* const newline_char = "\n";
    static const char* const space_char = " ";
    char temp[MAX_INT_LENGTH];
    APESEARCH::vector<size_t> titleWords;
    APESEARCH::vector<size_t> headingWords;
    APESEARCH::vector<size_t> boldWords;

    fillVectors(titleWords, headingWords, boldWords, parsed);

    size_t index = hash(parsed.url.cstr());
    index = index % file_vector.size();

    APESEARCH::unique_lock<APESEARCH::mutex> lk( file_vector[index].parsed_lock );
    file_vector[index].parsedFile.write(parsed.url.cstr(), parsed.url.size()); //url
    file_vector[index].parsedFile.write(newline_char, 1);

    for(size_t i = 0; i < parsed.parsed_text.size(); i++){ // parsed text
        file_vector[index].parsedFile.write(parsed.parsed_text[i].text.cstr(), parsed.parsed_text[i].text.size());
        file_vector[index].parsedFile.write(space_char, 1);
    }
    file_vector[index].parsedFile.write(newline_char, 1);

    writeIndex(titleWords, file_vector[index].parsedFile);
    writeIndex(headingWords, file_vector[index].parsedFile);
    writeIndex(boldWords, file_vector[index].parsedFile);

    file_vector[index].parsedFile.write(parsed.base.cstr(), parsed.base.size()); //base
    file_vector[index].parsedFile.write(newline_char, 1);

    size_t size = snprintf( temp, sizeof( temp ), "%d", parsed.numParagraphs); //num paragraphs
    file_vector[index].parsedFile.write(temp, size);
    file_vector[index].parsedFile.write(newline_char, 1);

    size = snprintf( temp, sizeof( temp ), "%d", parsed.numHeadings); //num headings
    file_vector[index].parsedFile.write(temp, size);
    file_vector[index].parsedFile.write(newline_char, 1);

    size = snprintf( temp, sizeof( temp ), "%d", parsed.numSentences); //num sentences
    file_vector[index].parsedFile.write(temp, size);
    file_vector[index].parsedFile.write(newline_char, 1);

    file_vector[index].parsedFile.write(null_char, 1); // null
}


void reduceAnchorMapFiles(int &fileCount){
    static const char* const newline_char = "\n";
    std::string path0 = "./anchorMapFiles0/anchorMapFile";
    std::string path1 = "./anchorMapFiles1/anchorMapFile";
    std::string path2 = "./anchorMapFiles2/anchorMapFile";
    

    for(int i = 0; i < fileCount; i++){
        std::string path = (i < 50000) ? path0 : (i < 100000) ? path1 : path2;
        reduceFile(path + std::to_string(i));
    }

}


void writePhrase( int fileCount, const std::string& phrase ){
    static const char* const newline_char = "\n";
    std::string path0 = "./anchorMapFiles0/anchorMapFile";
    std::string path1 = "./anchorMapFiles1/anchorMapFile";
    std::string path2 = "./anchorMapFiles2/anchorMapFile";


    std::string path = (fileCount < 50000) ? path0 : (fileCount < 100000) ? path1 : path2;
    path += std::to_string(fileCount);
    APESEARCH::File file( path.c_str(), O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
    file.write(phrase.c_str(), phrase.length());
    file.write(newline_char, 1);
}

void Database::parseAnchorFile(char const *anchorPtr, size_t fileSize, std::unordered_map<std::string, int> &anchorMap, int &fileCount){

    if(fileSize == 0){
        return;
    }
    
    char const *startingPtr = anchorPtr;
    while(anchorPtr < startingPtr + fileSize){

        std::string url = "";
        while(anchorPtr < startingPtr + fileSize && *anchorPtr != '\n'){
            url.push_back(*anchorPtr);
            anchorPtr++;
        }
        anchorPtr++;
        if(anchorPtr >= startingPtr + fileSize){
            //std::cerr << "Anchor was improperly formatted and went over filesize" <<std::endl;
            return;
        }

        std::string prot = url.substr(0, 5);
        if(prot != "https"){
            while(*anchorPtr != '\0'){
                anchorPtr++;
            }
            anchorPtr++;
            continue;
        }

        
        bool isAlphaNum = true;
        std::string phrase = "";
        while(anchorPtr < startingPtr + fileSize && *anchorPtr != '\n'){
            if(!std::isalnum(*anchorPtr) && *anchorPtr != ' '){
                isAlphaNum = false;
            }
            phrase.push_back(*anchorPtr);
            anchorPtr++;
        }
        anchorPtr++;
        if(!isAlphaNum){
            //std::cerr << "Anchor text was not alphanum" <<std::endl;
            while(anchorPtr < startingPtr + fileSize && *anchorPtr != '\0'){
                anchorPtr++;
            }
            anchorPtr++;
            continue;
        }
        if(anchorPtr >= startingPtr + fileSize){
            //std::cerr << "Anchor was improperly formatted and went over fileSize" <<std::endl;
            return;
        }
        if(*anchorPtr == '\0'){
            if(anchorMap.find(url) == anchorMap.end()){
                anchorMap[url] = fileCount++;
            }
            writePhrase(anchorMap[url], phrase);
        }
        else{
            //std::cerr << "Anchor was improperly formatted did not find a null where it was supposed to" <<std::endl;
            while(anchorPtr < startingPtr + fileSize && *anchorPtr != '\0'){
                anchorPtr++;
            }
        }
        if(anchorPtr >= startingPtr + fileSize){
            //std::cerr << "Anchor was improperly formatted amd went over filesize" <<std::endl;
            return;
        }
        
        assert(*anchorPtr == '\0');
        
        anchorPtr++;
    }
}

int verifyWords(char const *parsedPtr){
    double totalWords = 0;
    double numWord = 0;
    double nonAlphaNumWord = 0;

    char const *it = parsedPtr;
    std::string word = "";
    if(*it == '\n'){
        return 0;
    }
    while(it && *it != '\n'){
        bool isNum= true;
        bool isAlphaNum = true;
        while(*it != ' '){
            if(*it == '\n'){
                //std::cerr << "Parsed file incorrect format" << std::endl;
                return -1;
            }
            if(!std::isalnum(*it)){
                isAlphaNum = false;
            }
            if(!std::isdigit(*it)){
                isNum = false;
            }
            it++;
        }
        totalWords++;
        if(!isAlphaNum){
            nonAlphaNumWord++;
        }
        if(isNum){
            numWord++;
        }
        if(totalWords >= 100){
            break;
        }
        it++;
    }
    it++;
    if(numWord / totalWords >= .25 || nonAlphaNumWord / totalWords >= .25){
        return -1;
    }
    return it - parsedPtr;
}

void writeCondensedFile(std::string path, std::unordered_map<std::string, int> &anchorMap, char const *parsedPtr, int fileSize){
    std::string path0 = "./anchorMapFiles0/anchorMapFile";
    std::string path1 = "./anchorMapFiles1/anchorMapFile";
    std::string path2 = "./anchorMapFiles2/anchorMapFile";
    static const char* const null_char = "\0";

    APESEARCH::File condensedFile( path.c_str(), O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
    condensedFile.truncate(0);
    char const *it = parsedPtr;
    while(it && it < parsedPtr + fileSize){
        char const *beg = it;
        
        while(*it != '\n'){
            it++;
        }
        std::string url(beg, it);
        std::string prot = url.substr(0, 5);
        if(prot != "https"){
            anchorMap.erase(url);
            while(*it != '\0'){
                it++;
            }
            it++;
            continue;
        }
        it++;
        int parsedWordLen = verifyWords(it);
        if(parsedWordLen == -1){
            anchorMap.erase(url);
            while(*it != '\0'){
                it++;
            }
            it++;
            continue;
        }
        it += parsedWordLen;

        int numNewLine = 0;
        while(*it != '\0'){
            if(*it == '\n'){
                numNewLine++;
            }
            it++;
        }
        assert(*it == '\0');
        it++;
        if(numNewLine > 8){
            anchorMap.erase(url);
            continue;
        }
        std::string parsedInfo(beg, it);
        condensedFile.write(parsedInfo.c_str(), parsedInfo.length() - 1);
        if(anchorMap.find(url) != anchorMap.end()){
            std::string anchorMapPath = (anchorMap[url] < 50000) ? path0 : (anchorMap[url] < 100000) ? path1 : path2;
            anchorMapPath += std::to_string(anchorMap[url]);
            APESEARCH::File anchorMapFile( anchorMapPath.c_str(), O_RDWR , (mode_t) 0600 );
            if(anchorMapFile.fileSize() != 0){
                unique_mmap anchorMapMem( anchorMapFile.fileSize(), PROT_READ, MAP_SHARED, anchorMapFile.getFD(), 0 );
                char const *anchorMapPtr = reinterpret_cast< char const *>( anchorMapMem.get() );
                std::string anchorContent(anchorMapPtr, anchorMapPtr + anchorMapFile.fileSize());
                condensedFile.write(anchorContent.c_str(), anchorContent.length());
            }
            anchorMap.erase(url);
        }
        condensedFile.write(null_char, 1);
    }
    for(const auto &it : anchorMap){
        std::string writeString = it.first + "\n\n\n\n\n\n\n\n\n";
        std::string anchorMapPath = (anchorMap[it.first] < 50000) ? path0 : (anchorMap[it.first] < 100000) ? path1 : path2;
        anchorMapPath += std::to_string(anchorMap[it.first]);
        APESEARCH::File anchorMapFile( anchorMapPath.c_str(), O_RDWR , (mode_t) 0600 );
        if(anchorMapFile.fileSize() != 0){
            unique_mmap anchorMapMem( anchorMapFile.fileSize(), PROT_READ, MAP_SHARED, anchorMapFile.getFD(), 0 );
            char const *anchorMapPtr = reinterpret_cast< char const *>( anchorMapMem.get() );
            std::string anchorContent(anchorMapPtr, anchorMapPtr + anchorMapFile.fileSize());
            writeString += anchorContent;
            writeString.push_back('\0');
            condensedFile.write(writeString.c_str(), writeString.length());
        }
        
    }
}

void Database::condenseFile(APESEARCH::File &anchorFile, APESEARCH::File &parsedFile, int index){
    char *anchorPtr = nullptr; 
    char *parsedPtr = nullptr;
    unique_mmap anchorMem;

    if(anchorFile.fileSize() != 0){
        anchorMem = unique_mmap( anchorFile.fileSize(), PROT_READ, MAP_SHARED, anchorFile.getFD(), 0 );
        anchorPtr = reinterpret_cast< char *>( anchorMem.get() );
    }

    unique_mmap parsedMem;
    if(parsedFile.fileSize() != 0){
        parsedMem = unique_mmap( parsedFile.fileSize(), PROT_READ, MAP_SHARED, parsedFile.getFD(), 0 );
        parsedPtr = reinterpret_cast< char *>( parsedMem.get() );
    }
    

    std::unordered_map<std::string, int> anchorMap;
    assert(anchorMap.max_size() > 150000);
    int fileCount = 0;
    parseAnchorFile(anchorPtr, anchorFile.fileSize(), anchorMap, fileCount);
    reduceAnchorMapFiles(fileCount);
    writeCondensedFile("./condensedFiles/condensedFile" + std::to_string(index), anchorMap, parsedPtr, parsedFile.fileSize());
    cleanAnchorMap(fileCount);
}

void Database::condenseFiles(){

    for(int i = 0; i < file_vector.size(); i++){
        APESEARCH::unique_lock<APESEARCH::mutex> parsedLock( file_vector[i].parsed_lock );
        APESEARCH::unique_lock<APESEARCH::mutex> anchorLock( file_vector[i].anchor_lock );
        condenseFile(file_vector[i].anchorFile, file_vector[i].parsedFile, i);
    }
}

void Database::cleanAnchorMap(int fileCount){
    std::string path0 = "./anchorMapFiles0/anchorMapFile";
    std::string path1 = "./anchorMapFiles1/anchorMapFile";
    std::string path2 = "./anchorMapFiles2/anchorMapFile";

    for(int i = 0; i < fileCount; i++){
        std::string path = (i < 50000) ? path0 : (i < 100000) ? path1 : path2;
        path += std::to_string(i);
        APESEARCH::File file( path.c_str(), O_RDWR | O_CREAT | O_APPEND , (mode_t) 0600 );
        file.truncate(0);
    }
}




