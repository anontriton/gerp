# CXX is left at make's built-in default (c++ on macOS, g++ on Linux) so this
# builds with whatever compiler the platform ships. Override if you want a
# specific one, e.g.  make CXX=clang++
CXXFLAGS ?= -std=c++11 -g3 -Wall -Wextra -Wpedantic -Wshadow
LDFLAGS  ?= -g3

OBJS := main.o FileManager.o HashTable.o stringProcessing.o DirNode.o FSTree.o

default: gerp

gerp: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

main.o: main.cpp FileManager.h
FileManager.o: FileManager.cpp FileManager.h HashTable.h stringProcessing.h DirNode.h FSTree.h
HashTable.o: HashTable.cpp HashTable.h stringProcessing.h
stringProcessing.o: stringProcessing.cpp stringProcessing.h
DirNode.o: DirNode.cpp DirNode.h
FSTree.o: FSTree.cpp FSTree.h DirNode.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -rf gerp $(OBJS) *.dSYM
