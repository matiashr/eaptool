EXTERNAL=../

CFLAGS=\
	   -I$(EXTERNAL)/argh.git\
	   -g\
	   -I.\
	   -Iinclude/mcp/\
	   -I$(EXTERNAL)/json/include//\
	   -I$(EXTERNAL)/json/include/nlohmann/

EXTRALIBS=
LIBS=-lpthread -lrt  -lczmq -lzmq
OUTPUT=unittest
OUTPUTQ=queuetest
OUTPUTUTIL=rutil

LIBOBJS=\
	queue.o

OBJS= $(LIBOBJS)

COBJS=


-include $(OBJS:.o=.d)

%.o:%.c
	@echo  "CC $<"
	gcc -std=c99 -o $@ -I. -MT$@  -MD -MP -c $< 


%.o:%.cpp
	@echo  "CXX $<"
	@g++ $(CFLAGS) -std=c++17 -g -c -o $@ -MT$@ -MFaccesstest_accesstest.d -MD -MP $< 


all: $(OUTPUT)
	@echo make $(OUTPUT)

lib:
	ar -crs libmf.a $(LIBOBJS)

$(OUTPUTUTIL): $(OBJS) $(COBJS)
	@echo linking $@ 
	@g++ -g  ramdb.cpp -DCREATE_UTIL -Iinclude/mcp $(LIBS) $(EXTRALIBS) -o $(OUTPUTUTIL) 



#############################################################
# Unit tests
#############################################################
$(OUTPUT): $(OBJS) $(COBJS)
	@echo linking $@ $(COBJS) $(OBJS), link app with  -lczmq -lzmq
	@g++ -g $(OBJS) $(COBJS) $(LIBS) $(EXTRALIBS) -o $(OUTPUT) 

$(OUTPUTQ): $(OBJS) $(COBJS)
	@echo linking $@
	@g++ $(CFLAGS) -g -DUNITTEST queue.cpp  $(LIBS) $(EXTRALIBS) -o $(OUTPUTQ) 


clean:
	rm -f $(OBJS) $(OUTPUT) *.d libmf.a


