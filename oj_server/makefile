# oj_server:oj_server.cc
# 	g++ -o $@ $^ -I./include -L./lib -std=c++11 -lpthread -lctemplate -ljsoncpp -lmysqlclient
# .PHONY:clean
# clean:
# 	rm -f oj_server

oj_server:oj_server.cc
	g++ -o $@ $^ -I./include -I/usr/include/mysql -L./lib -std=c++11 -lpthread -lctemplate -ljsoncpp -lmysqlclient
.PHONY:clean
clean:
	rm -f oj_server