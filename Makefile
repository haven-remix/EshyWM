CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++1y
CXXFLAGS += `pkg-config --cflags x11 libglog`
LDFLAGS += `pkg-config --libs x11 libglog`

all: eshywm

HEADERS = \
    util.h \
    window_manager.h
SOURCES = \
    util.cpp \
    window_manager.cpp \
    eshywm.cpp
OBJECTS = $(SOURCES:.cpp=.o)

basic_wm: $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f eshywm $(OBJECTS)