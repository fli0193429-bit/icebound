CC = cc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDLIBS = -lm
COMMON = src/control.c src/mission.c
.PHONY: all test clean qnx evidence
all: build/icebound
build:
	mkdir -p build
build/icebound: $(COMMON) src/run.c src/control.h src/mission.h | build
	$(CC) $(CFLAGS) $(COMMON) src/run.c -o $@ $(LDLIBS)
build/test_control: src/control.c tests/test_control.c src/control.h | build
	$(CC) $(CFLAGS) -Isrc src/control.c tests/test_control.c -o $@ $(LDLIBS)
test: build/icebound build/test_control
	./build/test_control
	python3 tests/test_scenarios.py
qnx: | build
	qcc -Vgcc_ntox86_64 -std=c11 -O2 -Wall -Wextra -Werror $(COMMON) src/run.c -o build/icebound-qnx -lm
evidence: test
	python3 tools/report.py
	python3 tools/replay.py
clean:
	rm -rf build
