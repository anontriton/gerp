/*
 * Filename: DirNode.cpp
 * Contains: Implementation of the DirNode class declared in DirNode.h
 *
 * Note on ownership: DirNode has no destructor (see DirNode.h), so a node
 * does NOT own the subdirectory nodes it points at -- FSTree allocates the
 * whole tree and frees it in FSTree::burnTree. The copy constructor and
 * assignment operator therefore copy the subdirectory pointers rather than
 * cloning the subtree; deep copies go through FSTree::preOrderCopy, which
 * is what FSTree's own copy constructor and assignment use.
 */

#include "DirNode.h"

DirNode::DirNode() : name(""), parent(nullptr) {}

DirNode::DirNode(std::string newName) : name(newName), parent(nullptr) {}

DirNode::DirNode(const DirNode &other)
    : directories(other.directories), fileNames(other.fileNames),
      name(other.name), parent(other.parent) {}

DirNode &DirNode::operator=(const DirNode &other) {
    if (this != &other) {  // self-assignment is a no-op, not a crash
        directories = other.directories;
        fileNames = other.fileNames;
        name = other.name;
        parent = other.parent;
    }
    return *this;
}

void DirNode::addFile(std::string newName) {
    fileNames.push_back(newName);
}

void DirNode::addSubDirectory(DirNode *newDir) {
    directories.push_back(newDir);
}

bool DirNode::hasSubDir() {
    return not directories.empty();
}

bool DirNode::hasFiles() {
    return not fileNames.empty();
}

bool DirNode::isEmpty() {
    return directories.empty() and fileNames.empty();
}

int DirNode::numSubDirs() {
    return static_cast<int>(directories.size());
}

int DirNode::numFiles() {
    return static_cast<int>(fileNames.size());
}

void DirNode::setName(std::string newName) {
    name = newName;
}

std::string DirNode::getName() {
    return name;
}

DirNode *DirNode::getSubDir(int n) {
    if (n < 0 or n >= numSubDirs()) {
        return nullptr;
    }
    return directories[n];
}

std::string DirNode::getFile(int n) {
    if (n < 0 or n >= numFiles()) {
        return "";
    }
    return fileNames[n];
}

DirNode *DirNode::getParent() {
    return parent;
}

void DirNode::setParent(DirNode *newParent) {
    parent = newParent;
}
