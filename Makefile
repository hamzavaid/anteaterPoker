# Makefile for Anteater Poker

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS =

SRC_DIR = src
RULES_DIR = $(SRC_DIR)/rules
SERVER_DIR = $(SRC_DIR)/server
TEST_DIR = test
BIN_DIR = bin
BUILD_DIR = build

# Rule source files
RULES_SRC = \
	$(RULES_DIR)/cards.c \
	$(RULES_DIR)/deck.c

# Rule object files
RULES_OBJ = \
	$(BUILD_DIR)/cards.o \
	$(BUILD_DIR)/deck.o

# Server source files
SERVER_SRC = \
	$(SERVER_DIR)/server.c \
	$(SERVER_DIR)/game_state.c \
	$(SERVER_DIR)/socket_server.c

SERVER_OBJ = \
	$(BUILD_DIR)/server.o \
	$(BUILD_DIR)/game_state.o \
	$(BUILD_DIR)/socket_server.o

# Test files
TEST_DECK_SRC = $(TEST_DIR)/test_deck.c
TEST_DECK_BIN = $(BIN_DIR)/test_deck

# Executables
SERVER_BIN = $(BIN_DIR)/server

.PHONY: all test clean directories

all: directories $(SERVER_BIN) $(TEST_DECK_BIN)

directories:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BUILD_DIR)

# Rule object files
$(BUILD_DIR)/cards.o: $(RULES_DIR)/cards.c include/cards.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/deck.o: $(RULES_DIR)/deck.c include/deck.h include/cards.h
	$(CC) $(CFLAGS) -c $< -o $@

# Server object files
$(BUILD_DIR)/server.o: $(SERVER_DIR)/server.c include/game_state.h include/socket_server.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/game_state.o: $(SERVER_DIR)/game_state.c include/game_state.h include/cards.h include/deck.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/socket_server.o: $(SERVER_DIR)/socket_server.c include/socket_server.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build server executable
$(SERVER_BIN): $(SERVER_OBJ) $(RULES_OBJ)
	$(CC) $(CFLAGS) $(SERVER_OBJ) $(RULES_OBJ) -o $(SERVER_BIN) $(LDFLAGS)

# Build deck test executable
$(TEST_DECK_BIN): $(TEST_DECK_SRC) $(RULES_OBJ)
	$(CC) $(CFLAGS) $(TEST_DECK_SRC) $(RULES_OBJ) -o $(TEST_DECK_BIN) $(LDFLAGS)

test: all
	./$(TEST_DECK_BIN)

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(SERVER_BIN)
	rm -f $(TEST_DECK_BIN)