.phony all:
all: rsi

rsi: rsi.c
	gcc rsi.c -lreadline -o rsi

.PHONY clean:
clean:
	-rm -rf *.o *.exe
