## debug flag  
DBG_ENABLE   = 1  
## source file path  
## target exec file name  
#TARGET     := 
## get all source files  
##SRCS         += $(wildcard *.cpp)  
##SRCS_HEADER  += $(wildcard *.h)  
##SOURCES := $(filter-out %Test.cpp,$(SRCS))
#OBJS        := $(SOURCES:.cpp=.o)  

#LDFLAGS += -lpthread -lsqlite3 -ldl

#build:$(SOURCES) $(SRCS_HEADER)
#	g++ -c $(CXXFLAGS) $(SOURCES) -I ../../sqlite/ 
#	g++ -o $(TARGET) $(OBJS) $(LDFLAGS) -L ../../sqlite/ 
#
#
UTILS_SRC := utils/BasicHashtable.cpp \
	utils/BufferUtils.cpp \
	utils/Looper.cpp \
	utils/RefBase.cpp \
	utils/SharedBuffer.cpp \
	utils/StopWatch.cpp \
	utils/SystemClock.cpp \
	utils/Threads.cpp \
	utils/Timers.cpp \
	utils/VectorImpl.cpp \
	utils/WorkQueue.cpp \

MODEL_SRC := model/SocketAddress.cpp

STORE_SRC :=store/NetworkDatabase.cpp \
	store/SqliteWrapper.cpp

UTILS_OBJS        := $(UTILS_SRC:.cpp=.o)  
MODEL_OBJS        := $(MODEL_SRC:.cpp=.o)  
STORE_OBJS        := $(STORE_SRC:.cpp=.o)  

LDFLAGS += -lpthread -lsqlite3 -ldl

#build:$(SOURCES) $(SRCS_HEADER)
#	g++ -c $(CXXFLAGS) $(SOURCES) -I ../../sqlite/ 
#	g++ -o $(TARGET) $(OBJS) $(LDFLAGS) -L ../../sqlite/ 
#
#

networkdb_test:NetworkDBTest.cpp $(UTILS_OBJS) $(MODEL_OBJS) $(STORE_OBJS)
	g++ -o $@ NetworkDBTest.cpp $(UTILS_OBJS) $(MODEL_OBJS) $(STORE_OBJS) $(LDFLAGS) -L ../sqlite/ -I ./utils/ -I ./model -I ./store/

$(UTILS_OBJS):%.o : %.cpp
	g++ -c $< -o $@ $(CXXFLAGS) -I ../sqlite/  -I ./utils/

$(MODEL_OBJS):%.o : %.cpp
	g++ -c $< -o $@ $(CXXFLAGS) -I ./utils/

$(STORE_OBJS):%.o : %.cpp
	g++ -c $< -o $@ $(CXXFLAGS) -I ../sqlite/ -I ./utils/ -I ./model/

clean:
	rm -r utils/*.o model/*.o store/*.o
