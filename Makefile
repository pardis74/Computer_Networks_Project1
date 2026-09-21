CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET = dnsclient
SOURCES = src/main.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET) src/*.o

.PHONY: all clean