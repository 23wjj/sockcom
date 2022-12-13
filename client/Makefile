INCLUDE_DIR := $(CURDIR)/../include
SRC 		:= $(wildcard ./*.c) $(wildcard ./*.cpp)
OBJ 		:= $(patsubst %.cpp,%.o,$(SRC))
TARGET 		:= client

CC 			:=g++
CFLAGS      := -I$(INCLUDE_DIR) -lpthread

$(TARGET):$(OBJ)
	$(CC) -o $@ $^ -lpthread

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm $(TARGET)
	rm $(OBJ)
.PHONY:clean