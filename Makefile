CC=g++



all: my-router

my-router:
	$(CC) my-router.cpp -o my-router

clean:
	rm my-router