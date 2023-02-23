OBJS=main.o eth.o protocol.o
LIBS=-lpthread ../dependencies/moderncpp.git/libmf.a -lzmq

INCLUDE= -I ../dependencies/json/include/nlohmann/ \
    -I ../dependencies/json/include/\
    -I ../dependencies/moderncpp.git/include/\


%.o:%.c
	gcc -g $(INCLUDE) -c $< -o $@

%.o:%.cpp
	g++ -g $(INCLUDE) -c $< -o $@

eaptool:$(OBJS)
	g++ -g $(OBJS) $(LIBS) -o eaptool

all: eaptool

clean:
	rm -f *.o
	rm -f eaptool
