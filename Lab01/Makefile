CC =  gcc # Path to compiler
L_FLAGS = -lrt -lpthread -lm
#C_FLAGS = -g

all: ptA2 ptA3 
.PHONY: all

# Project A2 compilation
ptA2: periodicTaskA2.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)

# Project A3 compilation
ptA3: periodicTaskA3.c
	$(CC) $< -o $@ $(C_FLAGS) $(L_FLAGS)
	
.PHONY: clean 

clean:
	rm -f *.c~ 
	rm -f *.o
	rm ptA2
	rm ptA3

# Some notes
# $@ represents the left side of the ":"
# $^ represents the right side of the ":"
# $< represents the first item in the dependency list   

