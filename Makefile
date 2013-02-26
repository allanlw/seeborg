TGTDIR=./
FNAMEIRC=seeborg-irc
FNAMELINEIN=seeborg-linein

CFCPU = -march=native -mtune=native
CFOPT = -O2
CFUSER = -pthread

SRCS = seeborg.cpp seeutil.cpp

# -------
#
# -------
CC = gcc
CXX = g++
CFLAGS = $(CFCPU) $(CFOPT) $(CFDEBUG) $(CFUSER)
CXXFLAGS = $(CFLAGS)

SRC_IRC = $(FNAMEIRC).cpp $(wildcard botnet/*.c)
SRC_LINEIN = $(FNAMELINEIN).cpp

TGT_IRC = $(TGTDIR)/$(FNAMEIRC)
TGT_LINEIN = $(TGTDIR)/$(FNAMELINEIN)

OBJ_IRC = $(sort $(patsubst %.c,%.o,$(SRC_IRC:%.cpp=%.o)))
OBJ_LINEIN = $(sort $(patsubst %.c,%.o,$(SRC_LINEIN:%.cpp=%.o)))

DEP_IRC = $(sort $(patsubst %.c,%.d,$(SRC_IRC:%.cpp=%.d)))
DEP_LINEIN = $(sort $(patsubst %.c,%.d,$(SRC_LINEIN:%.cpp=%.d)))

OBJS = $(sort $(patsubst %.c,%.o,$(SRCS:%.cpp=%.o)))
DEPS = $(sort $(patsubst %.c,%.d,$(SRCS:%.cpp=%.d)))
DEPS += $(DEP_LINEIN) $(DEP_IRC)

all: compile

clean:
	rm -f $(TGT_IRC) $(TGT_LINEIN) $(OBJS) $(OBJ_IRC) $(OBJ_LINEIN) $(DEPS)

compile: makedirs $(DEPS) $(TGT_LINEIN) $(TGT_IRC)

$(TGT_IRC): $(OBJS) $(OBJ_IRC)
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(TGT_LINEIN): $(OBJS) $(OBJ_LINEIN)
	@echo Linking $@...
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.d: %.cpp
	@echo Updating $@...
	@$(CXX) $(CXXFLAGS) -MM $< -o $@

%.d: %.c
	@echo Updating $@...
	@$(CC) $(CFLAGS) -MM $< -o $@

makedirs:
	@if [ ! -d $(TGTDIR) ];then mkdir $(TGTDIR);fi

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
