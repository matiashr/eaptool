OBJS=main.o eth.o protocol.o
LIBS=-lpthread ../dependencies/moderncpp.git/libmf.a -lzmq 

FLAGS=-g -Wall
INCLUDE=\
	-I ../dependencies/json/include/nlohmann/ \
    -I ../dependencies/json/include/\
    -I ../dependencies/moderncpp.git/include/\


%.o:%.c
	gcc $(FLAGS) $(INCLUDE) -c $< -o $@

%.o:%.cpp
	g++ $(FLAGS) $(INCLUDE) -c $< -o $@

eaptool:$(OBJS)
	g++ $(FLAGS) $(OBJS) $(LIBS) -o eaptool

all: eaptool

clean:
	rm -f *.o
	rm -f eaptool
