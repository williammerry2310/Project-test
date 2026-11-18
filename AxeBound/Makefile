# AxeBound Makefile  (run "make" in this folder, then "make run")
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Iinclude
LDLIBS   := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
SRC      := $(wildcard src/*.cpp)
OBJ      := $(patsubst src/%.cpp,build/%.o,$(SRC))
TARGET   := AxeBound

$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDLIBS)

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

build:
	mkdir -p build

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build $(TARGET)

-include $(OBJ:.o=.d)
.PHONY: run clean
