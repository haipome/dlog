all:
	gcc -o dlog_test -g -Wall dlog.c test.c -D DEBUG -rdynamic
	gcc dlog_server.c dlog.c -D DLOG_SERVER -g -Wall -O2 -o dlog_server 

clean:
	rm -f dlog_test dlog_server
