sqlite3: 
	gcc -c sqlite3.c -o sqlite3.o 
	ar crv sqlite3.a sqlite3.o 

testbin:
	g++ sqlite_network.c sqlite_wrapper.c -lsqlite3 -L ./ -o testbin


