CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

antivirus: AntiVirus_Magshimim/prog.c
	$(CC) $(CFLAGS) -o antivirus AntiVirus_Magshimim/prog.c

test:
	./tests/run_tests.sh

clean:
	rm -f antivirus

.PHONY: test clean
