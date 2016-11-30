# $@	full target name
# $^	all dependacnies
# $?	out of date dependancies
# $<
# $*	target without suffix

%.class : %.java  ;	$(JAVAC) $(JFLAGS)  $<

#DEBUGFLAGS=-g -DDEBUG_TAG_SAVE -DDEBUG
DEBUGFLAGS=-g
CXXFLAGS=$(DEBUGFLAGS) -I/home/ray/tools/include/
LDFLAGS=$(DEBUGFLAGS) -L/opt/taglib/lib -Wl,-rpath=/opt/taglib/lib -ltag
LDFLAGS=$(DEBUGFLAGS) -L/home/ray/tools/lib -Wl,-rpath=/opt/taglib/lib -ltag -lz

TARGETS=mp3tag

all:	objs $(TARGETS)
objs:	mp3tag.o

mp3tag:	mp3tag.o
	$(CXX) $^ $(LDFLAGS) -o $@

mbstr:	mbstr.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

dispenc:	dispenc.cc
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

clean:
	rm -fr *.o core.* $(TARGETS) 
