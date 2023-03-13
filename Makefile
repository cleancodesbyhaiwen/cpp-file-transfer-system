CXX=g++
CXXFLAGS=-std=c++17 -pthread 

SOURCES=file.cpp client.cpp helper.cpp server.cpp
HEADERS=client.hpp helper.hpp server.hpp
OBJECTS=$(SOURCES:.cpp=.o)

file: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o file

$(OBJECTS): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $(SOURCES)

clean:
	rm -f $(OBJECTS) file *.txt