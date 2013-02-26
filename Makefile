#
#

TGTDIR = ./
#TGTDIR = ../bin
FNAMEIRC = seeborg-irc
FNAMELINEIN = seeborg-linein

CFCPU = -march=native -mtune=native
CFOPT = -O3 -fomit-frame-pointer -fforce-addr -finline -funroll-loops -fexpensive-optimizations
CFUSER = -pthread

#CFDEBUG = -g3
#CFDEBUG += -pg
#LDFLAGS = -s
LDFLAGS = -lwsock32

SRCS = seeborg.cpp seeutil.cpp

# -------
#
# -------
CC = gcc
CXX = g++
CFLAGS = $(CFCPU) $(CFOPT) $(CFDEBUG) $(CFUSER)
CXXFLAGS = $(CFLAGS)

SRC_IRC = $(FNAMEIRC).cpp botnet/botnet.c botnet/dcc_chat.c botnet/dcc_send.c botnet/output.c \
    botnet/server.c botnet/utils.c
SRC_LINEIN = $(FNAMELINEIN).cpp

TGT_IRC = $(TGTDIR)/$(FNAMEIRC)
TGT_LINEIN = $(TGTDIR)/$(FNAMELINEIN)

OBJ_IRCTMP = $(SRC_IRC:%.cpp=%.o)
OBJ_IRC = $(OBJ_IRCTMP:%.c=%.o)

OBJ_LINEINTMP = $(SRC_LINEIN:%.cpp=%.o)
OBJ_LINEIN = $(OBJ_LINEINTMP:%.c=%.o)

DEP_IRCTMP = $(SRC_IRC:%.cpp=%.d)
DEP_IRC = $(DEP_IRCTMP:%.c=%.d)

DEP_LINEINTMP = $(SRC_LINEIN:%.cpp=%.d)
DEP_LINEIN = $(DEP_LINEINTMP:%.c=%.d)

OBJSTMP = $(SRCS:%.cpp=%.o)
OBJS = $(OBJSTMP:%.c=%.o)

DEPSTMP = $(SRCS:%.cpp=%.d)
DEPS = $(DEPSTMP:%.c=%.d)

DEPS += $(DEP_LINEIN) $(DEP_IRC)

all: compile

clean:
	rm -f $(TGT_IRC) $(TGT_LINEIN) $(OBJS) $(OBJ_IRC) $(OBJ_LINEIN)
# $(DEPS)

compile: makedirs $(TGT_LINEIN) $(TGT_IRC)

$(TGT_IRC): $(OBJS) $(OBJ_IRC)
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) $(OBJS) $(OBJ_IRC) -o $@ $(LDFLAGS)

$(TGT_LINEIN): $(OBJS) $(OBJ_LINEIN)
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) $(OBJS) $(OBJ_LINEIN) -o $@ $(LDFLAGS)


.cpp.d:
	@echo Updating $@...
	@$(CXX) $(CXXFLAGS) -MM $< -o $@

.c.d:
	@echo Updating $@...
	@$(CC) $(CFLAGS) -MM $< -o $@

.cpp.o:
	@echo Compiling $@...
	@$(CXX) -c $< -o $@ $(CXXFLAGS)

.c.o:
	@echo Compiling $@...
	@$(CC) -c $< -o $@ $(CFLAGS)

makedirs:
	@if [ ! -d $(TGTDIR) ];then mkdir $(TGTDIR);fi
