CPPSRC = $(wildcard *.cpp) $(wildcard csscolorparser/*.cpp)
CSRC = $(wildcard *.c) $(wildcard xxhash/*.c) $(wildcard hpdf/*.c)
OBJ = $(CPPSRC:.cpp=.o) $(CSRC:.c=.o)
TARGET = chrd

FONTS = $(wildcard fonts/*.ttf)
FONTDATA = $(FONTS:.ttf=.inc)

ICON = icon/icon.png
ICONDATA = $(ICON:.png=.inc)

LANGS = $(wildcard syntax/*.lang)
RNGS = $(wildcard syntax/*.rng)
LANGDATA = $(LANGS:.lang=.inc) $(RNGS:.rng=.inc)

DUMMY = dummy/dummy.pdf
DUMMYDATA = $(DUMMY:.pdf=.inc)

LIBS = gtk+-3.0 evince-view-3.0 gtksourceview-3.0 gtk+-unix-print-3.0

COMMONFLAGS = -c -O3 -Wall -Wno-unused-parameter -D_GNU_SOURCE \
			  -D_FORTIFY_SOURCE=1 -I/usr/include/evince/3.0/ \
			  $(shell pkg-config --cflags $(LIBS)) -Ihpdf/include

CXX = clang++
CXXFLAGS = $(COMMONFLAGS) -std=c++11

CC = clang
CFLAGS = $(COMMONFLAGS) -std=c99 -Wno-unused-function

LD = clang++
LDFLAGS = -lm -lpthread -lpng -lz $(shell pkg-config --libs $(LIBS))

.PHONY: $(TARGET) clean run

all: debug

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

release: CXXFLAGS += -DNDEBUG
release: LDFLAGS += -s
release: $(TARGET)

$(TARGET): $(FONTDATA) $(ICONDATA) $(LANGDATA) $(DUMMYDATA) $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

%.inc: %.ttf
	xxd -i $< > $@

%.inc: %.png
	xxd -i $< > $@

%.inc: %.lang
	xxd -i $< > $@

%.inc: %.rng
	xxd -i $< > $@

%.inc: %.pdf
	xxd -i $< > $@

$(HPDF): get-deps

clean:
	rm -rf $(OBJ) $(TARGET) $(FONTDATA) $(ICONDATA) $(LANGDATA) $(DUMMYDATA)

cleanall: clean
	rm -rf deps

run: $(TARGET)
	./$(TARGET)

runv: $(TARGET)
	valgrind ./$(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/bin

uninstall:
	rm -f /usr/bin/$(TARGET)
