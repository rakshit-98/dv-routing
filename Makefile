CC=g++

all: my-router

my-router:
	$(CC) DV.cpp my-router.cpp -o my-router

clean:
	rm my-router