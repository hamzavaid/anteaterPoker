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

# Executables
SERVER_BIN = $(BIN_DIR)/server
TEST_DECK_BIN = $(BIN_DIR)/test_deck
TEST_SERVER_CLIENT_BIN = $(BIN_DIR)/test_server_client

# Source files
RULES_SRC = \
	$(RULES_DIR)/cards.c \
	$(RULES_DIR)/deck.c

SERVER_SRC = \
	$(SERVER_DIR)/server.c \
	$(SERVER_DIR)/game_state.c \
	$(SERVER_DIR)/socket_server.c

TEST_DECK_SRC = $(TEST_DIR)/test_deck.c
TEST_SERVER_CLIENT_SRC = $(TEST_DIR)/test_server_client.c

# Object files
RULES_OBJ = \
	$(BUILD_DIR)/cards.o \
	$(BUILD_DIR)/deck.o

SERVER_OBJ = \
	$(BUILD_DIR)/server.o \
	$(BUILD_DIR)/game_state.o \
	$(BUILD_DIR)/socket_server.o

.PHONY: all test server clean directories

all: directories $(SERVER_BIN) $(TEST_DECK_BIN) $(TEST_SERVER_CLIENT_BIN)

directories:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BUILD_DIR)

server: directories $(SERVER_BIN)

# General compile rule for rule files
$(BUILD_DIR)/%.o: $(RULES_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# General compile rule for server files
$(BUILD_DIR)/%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build server executable
$(SERVER_BIN): $(SERVER_OBJ) $(RULES_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build deck test executable
$(TEST_DECK_BIN): $(TEST_DECK_SRC) $(RULES_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

# Build server client test executable
$(TEST_SERVER_CLIENT_BIN): $(TEST_SERVER_CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

test: all
	./$(TEST_DECK_BIN)
	@echo ""
	@echo "Deck test finished."
	@echo "To test the server:"
	@echo "  Terminal 1: ./bin/server --port 10010 --table \"ZotHouse\""
	@echo "  Terminal 2: ./bin/test_server_client"

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(SERVER_BIN)
	rm -f $(TEST_DECK_BIN)
	rm -f $(TEST_SERVER_CLIENT_BIN)