CC = gcc
CFLAGS = -Wall -O2

TARGET = virt-regs

SRC = main.c virtual-registers.c

$(TARGET): $(SRC) 
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
