# Compiler
COMPILER=gcc-7

# Sqlite version (refer to https://www.sqlite.org/download.html for the last one)
SQLITE_VERSION=3340100
# Sqlite url
SQLITE_URL=https://sqlite.org/2021/sqlite-amalgamation-$(SQLITE_VERSION).zip

# Curl version (refer to https://curl.se/download.html for the last one)
CURL_VERSION=7.75.0
# Curl url
CURL_URL=https://curl.se/download/curl-$(CURL_VERSION).zip

# Build mode 0:dev, 1:prod
BUILD_MODE=0

# Compiler arguments depending on BUILD_MODE
ifeq ($(BUILD_MODE), 0)
	BUILD_ARG=-I./ -Wall -Wextra -Og -ggdb -g3 -DBUILDMODE=$(BUILD_MODE)
	LINK_ARG=-lm -lpthread -ldl -lcurl
else ifeq ($(BUILD_MODE), 1)
	BUILD_ARG=-I./ -Wall -Wextra -Werror -Wfatal-errors -O3 -DBUILDMODE=$(BUILD_MODE)
	LINK_ARG=-lm -lpthread -ldl -lcurl
endif

# Rules

all: main

main: sqlite3/sqlite3.o trycatch.o runrecorder.o main.o Makefile
	$(COMPILER) main.o runrecorder.o trycatch.o sqlite3/sqlite3.o $(LINK_ARG) -o main 

main.o: main.c runrecorder.h trycatch.h Makefile
	$(COMPILER) $(BUILD_ARG) -c main.c 

trycatch.o: trycatch.c trycatch.h Makefile
	$(COMPILER) -c trycatch.c

runrecorder.o: runrecorder.c runrecorder.h trycatch.h sqlite3/sqlite3.h curl/include/curl/curl.h Makefile
	$(COMPILER) $(BUILD_ARG) -c runrecorder.c

sqlite3/sqlite3.o: sqlite3/sqlite3.h
	cd sqlite3; \
	$(COMPILER) -O3 -c sqlite3.c; \
	cd -

sqlite3/sqlite3.h:
	rm -rf sqlite*; \
	wget $(SQLITE_URL); \
	unzip sqlite-amalgamation-$(SQLITE_VERSION).zip; \
	mv sqlite-amalgamation-$(SQLITE_VERSION) sqlite3; \
	rm sqlite-amalgamation-$(SQLITE_VERSION).zip

sqlite3/sqlite3:
	cd sqlite3; \
	$(COMPILER) shell.c sqlite3.c -lpthread -ldl -O3 -o sqlite3; \
	cd -

curl/include/curl/curl.h:
	rm -rf curl*; \
	wget $(CURL_URL); \
	unzip curl-$(CURL_VERSION).zip; \
	mv curl-$(CURL_VERSION) curl; \
	rm curl-$(CURL_VERSION).zip; \
	cd curl; \
	./configure; \
	make; \
	sudo make install; \
	cd -

clean:
	rm -rf *.o main sqlite* curl*

valgrind :
	valgrind -v --track-origins=yes --leak-check=full \
	--gen-suppressions=yes --show-leak-kinds=all ./main
