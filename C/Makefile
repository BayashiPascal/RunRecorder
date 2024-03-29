# Compiler
COMPILER=gcc

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
	BUILD_ARG=-std=c17 -I./ -pedantic -Wall -Wextra -Wno-clobbered -Og -ggdb -g3 -DBUILDMODE=$(BUILD_MODE)
	LINK_ARG=-lsqlite3 -lm -lpthread -ldl -lcurl -ltrycatchc
else ifeq ($(BUILD_MODE), 1)
	BUILD_ARG=-std=c17 -I./ -pedantic -Wall -Wextra -Werror -Wfatal-errors -Wno-clobbered -O3 -DBUILDMODE=$(BUILD_MODE)
	LINK_ARG=-lsqlite3 -lm -lpthread -ldl -lcurl -ltrycatchc
endif

# Rules

all: main runrecorder

main: runrecorder.o main.o Makefile
	$(COMPILER) main.o runrecorder.o $(LINK_ARG) -o main 

main.o: main.c runrecorder.h Makefile
	$(COMPILER) $(BUILD_ARG) -c main.c 

runrecorder: runrecorder.o cli.o Makefile
	$(COMPILER) cli.o runrecorder.o $(LINK_ARG) -o runrecorder 

cli.o: cli.c runrecorder.h Makefile
	$(COMPILER) $(BUILD_ARG) -c cli.c 

runrecorder.o: /usr/local/lib/libcurl.a \
	/usr/local/lib/libtrycatchc.a \
	/usr/local/lib/libsqlite3.a \
	runrecorder.c runrecorder.h Makefile
	$(COMPILER) $(BUILD_ARG) -c runrecorder.c

/usr/local/lib/libtrycatchc.a:
	wget https://github.com/BayashiPascal/TryCatchC/archive/main.zip
	unzip main.zip
	rm -rf TryCatchC
	mv TryCatchC-main TryCatchC
	rm main.zip
	cd TryCatchC && make && sudo make install
	rm -rf TryCatchC

/usr/local/lib/libcurl.a:
	wget $(CURL_URL)
	unzip curl-$(CURL_VERSION).zip
	rm -rf curl
	mv curl-$(CURL_VERSION) curl
	rm curl-$(CURL_VERSION).zip
	cd curl && ./configure && make && sudo make install
	rm -rf curl

/usr/local/lib/libsqlite3.a:
	wget $(SQLITE_URL)
	unzip sqlite-amalgamation-$(SQLITE_VERSION).zip
	rm -rf sqlite3
	mv sqlite-amalgamation-$(SQLITE_VERSION) sqlite3
	rm sqlite-amalgamation-$(SQLITE_VERSION).zip
	cd sqlite3 &&	$(COMPILER) -O3 -c sqlite3.c
	sudo rm -rf /usr/local/include/SQLite3
	sudo mkdir /usr/local/include/SQLite3
	cd sqlite3 && sudo cp sqlite3.h /usr/local/include/SQLite3/sqlite3.h
	sudo ar -r /usr/local/lib/libsqlite3.a sqlite3/sqlite3.o
	rm -rf sqlite3

clean:
	rm -f *.o main

clean_all: clean
	rm -rf sqlite* curl*

valgrind : main
	valgrind -v --track-origins=yes --leak-check=full \
	--gen-suppressions=yes --show-leak-kinds=all ./main

valgrindCli : runrecorder
	valgrind -v --track-origins=yes --leak-check=full \
	--gen-suppressions=yes --show-leak-kinds=all ./runrecorder runrecorder.db

install: runrecorder.o
	sudo rm -rf /usr/local/include/RunRecorder
	sudo mkdir /usr/local/include/RunRecorder
	sudo cp runrecorder.h /usr/local/include/RunRecorder/runrecorder.h
	sudo ar -r /usr/local/lib/librunrecorder.a runrecorder.o
	mkdir -p ~/Tools
	cp runrecorder ~/Tools/runrecorder
