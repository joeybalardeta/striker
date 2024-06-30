.DEFAULT_GOAL:=all

PROJECT_NAME=striker

SRC=src/
OBJ=obj/

# not using the BIN directory for storing the executable
# BIN=bin/

CC=gcc
CFLAGS=-Wall -std=c11
COMPILE=$(CC) $(CFLAGS)
OBJS:=$(patsubst $(SRC)%.c, $(OBJ)%.o, $(wildcard $(SRC)*.c))

init:
	mkdir -p $(OBJ)

$(OBJ)%.o: $(SRC)%.c
	$(COMPILE) -c $< -o $@

all: init $(OBJS)
	$(COMPILE) $(OBJS) -o $(PROJECT_NAME)

clean_obj:
	rm -f $(OBJ)*.o

clean:
	rm -f $(OBJ)*.o $(PROJECT_NAME)
