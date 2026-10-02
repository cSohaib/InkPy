EMBED = build/micropython_embed
CC ?= cc
CFLAGS = -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -fno-common -DNDEBUG
CFLAGS += -I. -I$(EMBED) -I$(EMBED)/port -I../../components/ink_python
VENDOR = $(wildcard $(EMBED)/*/*.c) $(wildcard $(EMBED)/*/*/*.c)
VENDOR := $(filter-out $(EMBED)/port/mphalport.c,$(VENDOR))
CORE = ../../components/ink_python/ink_python.c
all: build/inkpy-python build/inkpy-worker
build/inkpy-worker: $(VENDOR) $(CORE) worker.c mpconfigport.h
	$(CC) $(CFLAGS) $(VENDOR) $(CORE) worker.c -pthread -lm -o $@
build/inkpy-python: $(VENDOR) $(CORE) probe.c mpconfigport.h
	$(CC) $(CFLAGS) $(VENDOR) $(CORE) probe.c -lm -o $@
