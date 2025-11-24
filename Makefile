CC = g++
CFLAGS = -std=c++17 -g -fsanitize=address
SRC_DIR = ./src
OBJ_DIR = ./obj
INCLUDE_DIR = ./include
EXEC = interp

SRC = $(wildcard $(SRC_DIR)/*.cpp)
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

all: create_dirs $(EXEC)

create_dirs:
	$(shell mkdir -p $(OBJ_DIR))

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(CFLAGS) -I $(INCLUDE_DIR) -c $^ -o $@

.PHONY: clean
clean:
	rm -rf $(OBJ_DIR)/*.o $(EXEC)