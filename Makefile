SRCDIR := src
OBJDIR := obj
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

LIBONLY := Gem5Wrapper.cpp
SIMONLY := Main.cpp Cache.cpp Processor.cpp

# Ramulator currently supports g++ 5.1+ or clang++ 3.4+.  It will NOT work with
#   g++ 4.x due to an internal compiler error when processing lambda functions.
# CXX := clang++
# CXX := g++-5
CXXFLAGS := -O3 -std=c++11 -g -Wall
ARFLAGS := crus

.PHONY: all clean depend

all: depend ramulator libramulator.a

clean:
	rm -f ramulator libramulator.a
	rm -rf $(OBJDIR)

depend: $(OBJDIR)/.depend


$(OBJDIR)/.depend: $(SRCS)
	@mkdir -p $(OBJDIR)
	@rm -f $(OBJDIR)/.depend
	@$(foreach SRC, $(SRCS), $(CXX) $(CXXFLAGS) -DRAMULATOR -MM -MT $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC)) $(SRC) >> $(OBJDIR)/.depend ;)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJDIR)/.depend
endif


ramulator: $(filter-out $(addprefix $(OBJDIR)/, $(LIBONLY:.cpp=.o)), $(OBJS)) | depend
	$(CXX) $(CXXFLAGS) -DRAMULATOR -o $@ $^

libramulator.a: $(filter-out $(addprefix $(OBJDIR)/, $(SIMONLY:.cpp=.o)), $(OBJS)) | depend
	$(AR) $(ARFLAGS) $@ $^

$(OBJS): | $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -DRAMULATOR -c -o $@ $<
