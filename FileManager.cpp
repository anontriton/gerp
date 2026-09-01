#include "FileManager.h"

using namespace std;

FileManager::FileManager(const string &rootDir, const string &outputFilename)
    : fileSystem(rootDir), outputFile(outputFilename) {
    indexFiles(fileSystem.getRoot(), rootDir);
}

FileManager::~FileManager() {
    if (outputFile.is_open()) {
        outputFile.close();
    }
}

void FileManager::indexFiles(DirNode *currNode, const string &currentPath) {
    if (currNode == nullptr) {
        return;
    }

    for (int i = 0; i < currNode->numFiles(); ++i) {
        string filePath = currentPath + "/" + currNode->getFile(i);

        ifstream inFile(filePath);
        string line;
        int lineNum = 1;

        while (getline(inFile, line)) {
            istringstream iss(line);
            string word;

            while (iss >> word) {
                string sw = stripNonAlphaNum(word); // sw stands for stripped word
                string entry = filePath + ":" + to_string(lineNum) + ": " + line;

                // Exact-case index, for case-sensitive queries.
                ht.insert(sw, entry);

                // Separate lowercase-keyed index, so case-insensitive queries
                // can aggregate occurrences regardless of original casing
                // without exact-case searches picking up other-case matches.
                htLower.insert(toLowercase(sw), entry);
            }
            ++lineNum;
        }

        inFile.close();
    }

    for (int i = 0; i < currNode->numSubDirs(); ++i) {
        DirNode* subDir = currNode->getSubDir(i);
        indexFiles(subDir, currentPath + "/" + subDir->getName());
    }
}

void FileManager::processQueries() {
    string query;

    while (true) {
        cout << "Query? ";

        getline(cin, query);

        if (query == "@q") {
            cout << "Goodbye! Thank you and have a nice day." << endl;
            exit(0);

        } else if (query == "@quit") {
            cout << "Goodbye! Thank you and have a nice day." << endl;
            exit(0);

        } else if (query == "@f") {
            string newOutputFile;
            cin >> newOutputFile;
            // cin >> leaves the trailing '\n' in the buffer; without
            // discarding it, the next getline(cin, query) immediately reads
            // an empty line as a spurious query.
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (outputFile.is_open()) {
                outputFile.close();
            }
            outputFile.open(newOutputFile);

        } else if (query.substr(0,12) == "@insensitive") {
            // Must be checked before the shorter "@i" prefix below, since
            // "@insensitive" also starts with "@i" and would otherwise
            // always be matched (and mis-stripped) by that branch first.
            query = removeCommand(query);
            performQuery(query, false);

        } else if (query.substr(0,2) == "@i") {
            query = removeCommand(query);
            performQuery(query, false);

        } else {
            performQuery(query, true);
        }
    }
}

void FileManager::performQuery(const string &query, bool caseSensitive) {
    string strippedQuery = stripNonAlphaNum(query);

    string result;
    if (caseSensitive) {
        result = ht.search(strippedQuery);
    } else {
        strippedQuery = toLowercase(strippedQuery);
        result = htLower.search(strippedQuery);
    }

    if (not result.empty()) {
        processResults(result);
    } else {
        outputFile << strippedQuery;
        queryNotFound(caseSensitive);
    }
}

void FileManager::processResults(const string &result) {
    istringstream iss(result);
    string record;

    // Each record is "filePath:lineNum: line text"; splitting on whitespace
    // would break as soon as the matched line contains more than one word,
    // so locate the two delimiter colons by position instead.
    while (getline(iss, record)) {
        size_t firstColon = record.find(':');
        size_t secondColon = record.find(':', firstColon + 1);
        if (firstColon == string::npos || secondColon == string::npos) {
            continue;
        }

        string filePath = record.substr(0, firstColon);
        int lineNum = stoi(record.substr(firstColon + 1, secondColon - firstColon - 1));
        string line = record.substr(secondColon + 2); // skip ": "

        printResults(filePath, lineNum, line);
    }
}

void FileManager::printResults(const string &filePath, int lineNum,
                               const string &line) {
    outputFile << filePath << ":" << lineNum << ": " << line << endl;
}

void FileManager::queryNotFound(bool caseSensitive) {
    if (not caseSensitive) {
        outputFile << " Not Found." << endl;
    } else {
        outputFile << " Not Found. Try with @insensitive or @i." << endl;
    }
}

string FileManager::toLowercase(const string &input) {
    string result = input;
    for (char &c : result) {
        c = std::tolower(c);
    }
    return result;
}

string FileManager::removeCommand(const string &query) {
    string strippedQuery;
    if (query.substr(0,12) == "@insensitive") {
        strippedQuery = query.substr(12);
    } else if (query.substr(0,2) == "@i") {
        strippedQuery = query.substr(2);
    }
    return strippedQuery;
}