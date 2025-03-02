SOURCE := main.c http/http_lib.c http/http_data_structures.c
CC_FLAGS := -Wall -fsanitize=address -ggdb -DDEBUG -pthread
TARGET := out/server

compile: create_dir
	gcc $(CC_FLAGS) -o $(TARGET) $(SOURCE)

create_dir:
	mkdir -p out