CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -fopenmp
TARGET ?= nbody_omp.exe
SRC := main.cpp ../core/io.cpp ../core/physics.cpp 
INCLUDES := -I../core

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET)

run: $(TARGET)
	.\$(TARGET)

clean:
	del /Q $(TARGET) 2>NUL || exit 0
