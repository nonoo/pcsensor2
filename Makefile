EXECUTABLE=pcsensor2

all: $(EXECUTABLE)

$(EXECUTABLE): *.c
	gcc -O4 -Wall pcsensor2.c -o $(EXECUTABLE) -lusb-1.0

clean:
	rm -f $(EXECUTABLE)
