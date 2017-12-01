## debug flag  
DBG_ENABLE   = 1  
## source file path  
## target exec file name  
TARGET     := test  
## get all source files  
SRCS         += $(wildcard *.cpp)  
SRCS_HEADER  += $(wildcard *.h)  
OBJS        := $(SRCS:.cpp=.o)  

build:$(SRCS) $(SRCS_HEADER)
	g++ -c $(CFLAGS) $(SRCS)  
	g++ -o $(TARGET) $(OBJS) $(LDFLAGS)  
