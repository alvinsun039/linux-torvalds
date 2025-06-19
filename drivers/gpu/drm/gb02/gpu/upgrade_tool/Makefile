#VERSION  =1.0.0
CC = gcc
DEBUG = -DUSE_DEBUG
CFLAGS = -Wall
SOURCES = $(wildcard ./source/*.c)
INCLUDES  = -I./include
LIB_NAMES = -lssl -lcrypto
OBJ =$(patsubst %.c, %.o, $(SOURCES))
TARGET = upgrade
#links
$(TARGET):$(OBJ)
	@mkdir -p bin
	$(CC) $(OBJ) $(LIB_PATH) $(LIB_NAMES) -o bin/$(TARGET)
	@rm -rf $(OBJ)
	    
#compile
%.o:%.c
	$(CC) $(INCLUDES) $(DEBUG) -c $(CFLAGS) $< -o $@
.PHONY:clean
clean:
	@echo "Remove linked and compiled files......"
	rm -rf $(OBJ) $(TARGET) bin
