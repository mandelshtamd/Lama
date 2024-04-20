FLAGS=-m32 -g2 -fstack-protector-all

all: byterun

byterun: byterun.o common.o
	$(CC) $(FLAGS) -o byterun byterun.o common.o

byterun.o: byterun.c common.h
	$(CC) $(FLAGS) -g -c byterun.c

common.o: common.c common.h
	$(CC) $(FLAGS) -g -c common.c

clean:
	$(RM) *.a *.o *~ byterun
