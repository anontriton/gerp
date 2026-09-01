#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "stringProcessing.h"
#include "HashTable.h"
#include "DirNode.h"
#include "FSTree.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

using namespace std;

class FileManager {
public:
    FileManager(const string &rootDir, const string &outputFilename);
    ~FileManager();

    void processQueries();

private:
    FSTree fileSystem;
    HashTable ht; // ht stands for hash table (exact-case keys)
    HashTable htLower; // lowercase-keyed index, used for case-insensitive queries
    ofstream outputFile;

    void indexFiles(DirNode *currNode, const string &parentPath);
    void performQuery(const string &query, bool caseSensitive);
    void processResults(const string &result);
    void printResults(const string &filePath, int lineNum, const string &line);
    void queryNotFound(bool caseSensitive);
    
    // helper functions
    string toLowercase(const string &input);
    string removeCommand(const string &query);
};

#endif // FILEMANAGER_H
