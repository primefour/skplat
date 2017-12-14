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

networkdb_test: $(UTILS_OBJS) $(MODEL_OBJS) $(STORE_OBJS)
	g++ -o $@ $(UTILS_OBJS) $(MODEL_OBJS) $(STORE_OBJS) $(LDFLAGS) -L ../sqlite/

$(UTILS_OBJS):$(UTILS_SRC) 
	g++ -c $(CXXFLAGS) $(UTILS_SRC) -I ../sqlite/  -I ./utils/

$(MODEL_OBJS):$(MODEL_SRC) 
	g++ -c $(CXXFLAGS) $(MODEL_SRC) -I ./utils/

$(STORE_OBJS):$(STORE_SRC) 
	g++ -c $(CXXFLAGS) $(STORE_SRC) -I ../sqlite/ -I ./utils/ -I ./model/



