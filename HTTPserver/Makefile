SOURCE := main.c http_lib.c
CC_FLAGS := -Wall -fsanitize=address -ggdb -DDEBUG
TARGET := server

compile:
	gcc $(CC_FLAGS) -o $(TARGET) $(SOURCE)