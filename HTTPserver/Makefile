SOURCE := main.c http/http_lib.c http/http_data_structures.c
CC_FLAGS := -Wall -fsanitize=address -ggdb -DDEBUG -pthread
TARGET := server

compile:
	gcc $(CC_FLAGS) -o $(TARGET) $(SOURCE)