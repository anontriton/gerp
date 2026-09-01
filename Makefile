CXX      = clang++
CXXFLAGS = -g3 -Wall -Wextra -Wpedantic -Wshadow
# -no-pie: DirNode.o/FSTree.o are precompiled non-PIE objects (no source
# available); modern linkers default to PIE and refuse to link them without it.
LDFLAGS  = -g3 -no-pie

default: gerp

gerp: main.o FileManager.o HashTable.o stringProcessing.o DirNode.o FSTree.o
	$(CXX) $(LDFLAGS) -o gerp main.o FileManager.o HashTable.o \
	                     stringProcessing.o DirNode.o FSTree.o

main.o: main.cpp FileManager.h
	$(CXX) $(LDFLAGS) -c main.cpp

FileManager.o: FileManager.cpp FileManager.h
	$(CXX) $(LDFLAGS) -c FileManager.cpp

HashTable.o: HashTable.cpp HashTable.h
	$(CXX) $(LDFLAGS) -c HashTable.cpp

stringProcessing.o: stringProcessing.cpp stringProcessing.h
	$(CXX) $(LDFLAGS) -c stringProcessing.cpp

clean:
	rm -rf gerp main.o HashTable.o FileManager.o stringProcessing.o
