EXTS = .h .H .hh .hpp .HPP .h++ .hxx .hp
SRCEXTS = .c .C .cc .cpp .CPP .c++ .cxx .cp
CXXFLAGS=g++ -g
INCLUDE=/home/caoge/boost/include
MY_LIBS=-lboost_system -lboost_filesystem -lboost_thread -lpthread -lrt
MY_LINK=-L/home/caoge/boost/lib
CFLAGS=gcc
exe=http_server
cpp=test-xmon.cpp http_server.cpp io_service_poll.cpp session.cpp 
obj=test-xmon.o  http_server.o io_service_poll.o session.o

$(exe):$(obj)
	@echo "link start............"
	$(CXXFLAGS) -I$(INCLUDE) $(MY_LINK) -o $(exe) $(obj) $(MY_LIBS)
test-xmon.o:$(cpp)
	@echo "compile start......."
	$(CXXFLAGS) -c $(cpp)
.PHONY:clean delete
all:
	@echo "start make all.........."
clean:
	@echo "start clean............."
	-rm -rf $(obj) $(exe)
delete:
	@echo "delete.................."
	pwd

