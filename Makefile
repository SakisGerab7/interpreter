CC = g++
CFLAGS = -std=c++17
SRC_DIR = ./src
INCLUDE_DIR = ./include

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, %.o, $(SRC))

all: interp clean

interp: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $^ -o $@

clean:
	rm $(OBJ)