SOURCE := main.c http/http_lib.c http/http_data_structures.c cgi/cgi_interface.c c_log/c_log.c
CC_FLAGS := 
CC_DEV_FLAGS := -Wall -fsanitize=address -ggdb -pthread 
TARGET := out/server

compile: create_dir
	gcc $(CC_FLAGS) -o $(TARGET) $(SOURCE)

.PHONY: clean, dev

clean:
	rm -r -f out/

dev: create_dir
	gcc $(CC_DEV_FLAGS) -o $(TARGET) $(SOURCE)

create_dir:
	mkdir -p out