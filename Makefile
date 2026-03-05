CXX      := g++
CC       := gcc
CXXFLAGS := -Wall -std=c++17 -O2 -Iinclude
CFLAGS   := -Wall -O2 -Iinclude
LIBS     := -lglfw -lGL -lGLU -lm -ldl -lpthread

# 1. Always compile glad.c into an object file
GLAD_OBJ := src/glad.o

src/glad.o: src/glad.c
	$(CC) $(CFLAGS) -c $< -o $@

# 2. Universal rule: 'make x' looks for 'src/x.cpp' 
# It then links 'x.o' and 'glad.o' together.
%: src/%.cpp $(GLAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f src/*.o main test