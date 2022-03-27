.SECONDARY:

all: test-data test-multiargs test-rodata test-recurse

common.o:
	gcc common.c -c -m32 -no-pie -o common.o

test-%: %-32.c %-64-32.o common.o
	gcc $^ -m32 -no-pie -o $@

%-64.o: %-64.c
	gcc -fno-pic -mcmodel=small -fno-common -fno-stack-protector -c $<

%-64-32.o: converter %-64.o %.flist
	./converter $(filter-out $<,$^) $@

clean:
	rm -f *.o test-*
