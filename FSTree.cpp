/*
 * Filename: FSTree.cpp
 * Contains: Implementation of the FSTree class declared in FSTree.h
 *
 * Builds an in-memory n-ary tree mirroring a directory on the real
 * filesystem. Uses POSIX dirent/stat, which covers macOS and Linux.
 *
 * Naming convention (matches the behavior the rest of the program expects):
 * the ROOT node's name is the path string exactly as it was passed in --
 * e.g. FSTree("some/dir") gives a root named "some/dir" -- while every
 * subdirectory node's name is just its own basename, e.g. "nested". This
 * lets a caller rebuild any node's full path by joining the root path with
 * the basenames encountered on the way down.
 */

#include "FSTree.h"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <string>
#include <vector>

FSTree::FSTree() : root(nullptr) {}

FSTree::FSTree(std::string rootName) : root(buildTree(rootName)) {}

FSTree::FSTree(const FSTree &other) : root(nullptr) {
    if (other.root != nullptr) {
        root = preOrderCopy(other.root, nullptr);
    }
}

FSTree::~FSTree() {
    burnTree();
}

FSTree &FSTree::operator=(const FSTree &other) {
    if (this != &other) {
        burnTree();
        if (other.root != nullptr) {
            root = preOrderCopy(other.root, nullptr);
        }
    }
    return *this;
}

DirNode *FSTree::getRoot() const {
    return root;
}

bool FSTree::isEmpty() {
    return root == nullptr;
}

void FSTree::burnTree() {
    burnTree(root);
    root = nullptr;
}

// Post-order delete: free every child before freeing the node itself, so no
// node is read after it has been deleted.
void FSTree::burnTree(DirNode *currNode) {
    if (currNode == nullptr) {
        return;
    }

    for (int i = 0; i < currNode->numSubDirs(); i++) {
        burnTree(currNode->getSubDir(i));
    }

    delete currNode;
}

bool FSTree::is_file(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return false;
    }
    return S_ISREG(info.st_mode);
}

bool FSTree::is_dir(const char *path) {
    struct stat info;
    if (stat(path, &info) != 0) {
        return false;
    }
    return S_ISDIR(info.st_mode);
}

std::string FSTree::baseName(std::string const &path) {
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}

DirNode *FSTree::buildTree(std::string rootName) {
    // The node keeps the string it was built from; callers rename
    // subdirectory nodes to their basename below.
    DirNode *currNode = new DirNode(rootName);

    DIR *dir = opendir(rootName.c_str());
    if (dir == nullptr) {
        return currNode;  // unreadable or not a directory: leave it empty
    }

    // Collect entries first, then sort them. readdir order is filesystem
    // dependent (and differs between APFS and ext4), so sorting keeps the
    // traversal -- and therefore gerp's output -- identical across machines.
    std::vector<std::string> entries;
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string entryName = entry->d_name;
        if (entryName == "." or entryName == "..") {
            continue;
        }
        entries.push_back(entryName);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    for (const std::string &entryName : entries) {
        std::string path = rootName + "/" + entryName;

        if (is_dir(path.c_str())) {
            DirNode *subDir = buildTree(path);
            subDir->setName(baseName(path));
            subDir->setParent(currNode);
            currNode->addSubDirectory(subDir);
        } else if (is_file(path.c_str())) {
            currNode->addFile(entryName);
        }
        // anything else (symlink to nowhere, socket, fifo) is skipped
    }

    return currNode;
}
