.DEFAULT_GOAL:=all

PROJECT_NAME=striker

SRC=src/
OBJ=obj/
ARCHIVE=archive/

# not using the BIN directory for storing the executable
# BIN=bin/

CC=gcc
CFLAGS=-Wall -std=c11 -O3 -march=native -flto -Isrc
LFLAGS=-lrt
COMPILE=$(CC) $(CFLAGS)
OBJS:=$(patsubst $(SRC)%.c, $(OBJ)%.o, $(wildcard $(SRC)*.c))

# the archived mailbox generator is compiled in temporarily as the --crosscheck
# reference oracle; remove this once the bitboard generator is fully proven
OBJS+=$(OBJ)rules_mailbox.o

init:
	mkdir -p $(OBJ)

$(OBJ)%.o: $(SRC)%.c
	$(COMPILE) -c $< -o $@

$(OBJ)rules_mailbox.o: $(ARCHIVE)rules_mailbox.c
	$(COMPILE) -c $< -o $@

all: init $(OBJS)
	$(COMPILE) $(OBJS) -o $(PROJECT_NAME) $(LFLAGS)

clean_obj:
	rm -f $(OBJ)*.o

clean:
	rm -f $(OBJ)*.o $(PROJECT_NAME)
