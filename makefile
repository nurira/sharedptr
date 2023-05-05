CFLAGS = -std=c++11 -lpthread -o main -O4

all: main

main: SharedPtr_test.cpp SharedPtr.hpp
	g++ $(CFLAGS) SharedPtr_test.cpp

clean:
	rm -f main
