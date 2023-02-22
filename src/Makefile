OBJS=main.o eth.o protocol.o

%.o:%.c
	gcc -g -c $< -o $@

%.o:%.cpp
	g++ -g -c $< -o $@

eaptool:$(OBJS)
	g++ -g $(OBJS) -o eaptool

all: eaptool

clean:
	rm -f *.o
	rm -f eaptool
