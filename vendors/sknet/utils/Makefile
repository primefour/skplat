## debug flag  
DBG_ENABLE   = 1  
## source file path  
## target exec file name  
TARGET     := test  
## get all source files  
SRCS         += $(wildcard *.cpp)  
SRCS_HEADER  += $(wildcard *.h)  
SOURCES := $(filter-out %Test.cpp,$(SRCS))
OBJS        := $(SOURCES:.cpp=.o)  

LDFLAGS += -lpthread

build:$(SOURCES) $(SRCS_HEADER)
	g++ -c $(CXXFLAGS) $(SOURCES)  
	g++ -o $(TARGET) $(OBJS) $(LDFLAGS)  

log_test:LogTest.cpp
	g++ Log.h LogTest.cpp -o LogTest
atomic_test:AtomicTest.cpp
	g++ AtomicTest.cpp -o AtomicTest
timer_test:TimerTest.cpp Timers.cpp
	g++ TimerTest.cpp Timers.cpp -o TimerTest
refbase_test:RefBase.cpp RefBaseTest.cpp
	g++ RefBase.cpp ./RefBaseTest.cpp -o RefBaseTest
list_test:ListTest.cpp
	g++ ListTest.cpp -o ListTest
share_buffer:SharedBuffer.cpp SharedBufferTest.cpp
	g++ SharedBuffer.cpp SharedBufferTest.cpp -o SharedBufferTest
thread_test:Threads.cpp ThreadsTest.cpp RefBase.cpp
	g++ Threads.cpp ThreadsTest.cpp RefBase.cpp -o ThreadsTest -lpthread
