runtest: test
	rm -f *.gcda
	./test --verbose
	gcovr -s --html --html-details --output coverage.html

test: CFLAGS += -Wall -fprofile-arcs -ftest-coverage -g -O0
test: LDLIBS += -lcriterion -lgcov
test: rtic.c test.c

example: CFLAGS += -Wall
example: rtic.c example.c

clean:
	rm -Rf *.o *.gc* test example coverage*.html

.PHONY: runtest clean
