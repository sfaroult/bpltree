CFLAGS=-Wall
OBJFILES= bpltree.o bpltree_op.o bpltree_ins.o \
		  bpltree_del.o bpltree_search.o \
		  bpltree_err.o btplus.o debug.o
#LIBS= -lefence

all: bpltree

btplus.c : genkw keywords.txt
	sort -u keywords.txt | genkw btplus

btplus.h: btplus.c 

bpltree: $(OBJFILES)
	gcc -o bpltree $(OBJFILES) $(LIBS)

%.o:%.c
	gcc $(CFLAGS) -c -g $< -o $@

bpltree.o: btplus.h bpltree.c
	gcc -c -o bpltree.o bpltree.c

genkw: genkw.c
	gcc -o genkw genkw.c

clean:
	-rm bpltree
	-rm genkw
	-rm *.o
