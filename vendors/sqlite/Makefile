testbin:
	g++ sqlite_network.c sqlite_network_xhost.cc sqlite_network_xdns.cc  sqlite_wrapper.c -L . -lsqlite3  -lpthread -ldl  -o testbin

sqlite3: 
	gcc -c sqlite3.c -o sqlite3.o 
	ar crv sqlite3.a sqlite3.o 

clean:
	rm -r testbin



