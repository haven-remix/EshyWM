CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++20
CXXFLAGS += `pkg-config --cflags x11 libglog`
LDFLAGS += `pkg-config --libs x11 libglog`

all: eshywm

HEADERS = \
    util.h \
    config.h \
    window.h \
    window_manager.h \
    eshywm.h
SOURCES = \
    util.cpp \
    window.cpp \
    window_manager.cpp \
    eshywm.cpp \
    main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

eshywm: $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f eshywm $(OBJECTS)