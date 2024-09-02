# oj_server:oj_server.cc	
# 	g++ -o $@ $^ /usr/local/lib/libredis++.a /usr/lib/x86_64-linux-gnu/libhiredis.a -pthread -I./include -L./lib -L./libmysqlclient.so.18 -std=c++17 -lpthread -lctemplate -ljsoncpp -lmysqlclient 
# .PHONY:clean
# clean:
# 	rm -f oj_server

oj_server: oj_server.cc
	g++ -o $@ $^ /usr/local/lib/libredis++.a /usr/lib/x86_64-linux-gnu/libhiredis.a -pthread -I./include -I/usr/local/mysql/include -L./lib -L/usr/lib64 -std=c++17 -lpthread -lctemplate -ljsoncpp -lmysqlclient

.PHONY: clean
clean:
	rm -f oj_server