# Compiler
COMPILER=gcc-7

# Sqlite version
SQLITE_VERSION=3340100
SQLITE_ZIP=https://sqlite.org/2021/sqlite-amalgamation-$(SQLITE_VERSION).zip

# Build mode
BUILD_MODE=0

# Compiler arguments depending on BUILD_MODE
ifeq ($(BUILD_MODE), 0)
	BUILD_ARG=\
		-I./sqlite-amalgamation-$(SQLITE_VERSION) -Wall -Wextra -Og -ggdb -g3 \
		-DBUILDMODE=$(BUILD_MODE)
	LINK_ARG=-lm -lpthread -ldl
else ifeq ($(BUILD_MODE), 1)
	BUILD_ARG=\
		-I./sqlite-amalgamation-$(SQLITE_VERSION) -Wall -Wextra -Werror \
		-Wfatal-errors -O3 -DPBERRSAFEMALLOC='1' -DPBERRSAFEIO='1' \
		-DBUILDMODE=$(BUILD_MODE)
	LINK_ARG=-lm -lpthread -ldl
endif

# Rules

all: main

main: main.o runrecorder.o sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.o Makefile
	$(COMPILER) main.o runrecorder.o sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.o $(LINK_ARG) -o main 

main.o: main.c runrecorder.h sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.h Makefile
	$(COMPILER) $(BUILD_ARG) -c main.c 

runrecorder.o: runrecorder.c runrecorder.h sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.h Makefile
	$(COMPILER) $(BUILD_ARG) -c runrecorder.c

sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.o: sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.h
	cd sqlite-amalgamation-$(SQLITE_VERSION); \
	$(COMPILER) -O3 -c sqlite3.c; \
	cd -

sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3.h:
	rm -rf sqlite-amalgamation-*; \
	wget $(SQLITE_ZIP); \
	unzip sqlite-amalgamation-$(SQLITE_VERSION).zip; \
	rm -rf sqlite-amalgamation-*.zip

sqlite-amalgamation-$(SQLITE_VERSION)/sqlite3:
	cd sqlite-amalgamation-$(SQLITE_VERSION); \
	$(COMPILER) shell.c sqlite3.c -lpthread -ldl -O3 -o sqlite3; \
	cd -

clean:
	rm -rf *.o main sqlite-amalgamation-*
