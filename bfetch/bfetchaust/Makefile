AUSTRAL_BIN := ../austral/_build/default/bin/austral.exe
ENTRY := BfetchAust.Fetch:main
OUT := bfetchaust
OUT_C := $(OUT).c
CC := cc
CFLAGS := -O3 -march=native -fwrapv -Wno-builtin-declaration-mismatch
SRC_FILES := \
	runtime/BfetchAust.Posix.aui runtime/BfetchAust.Posix.aum \
	runtime/BfetchAust.FastIO.aui runtime/BfetchAust.FastIO.aum \
	runtime/BfetchAust.Sys.aui runtime/BfetchAust.Sys.aum \
	src/Fetch.aum
SRC := \
	runtime/BfetchAust.Posix.aui,runtime/BfetchAust.Posix.aum \
	runtime/BfetchAust.FastIO.aui,runtime/BfetchAust.FastIO.aum \
	runtime/BfetchAust.Sys.aui,runtime/BfetchAust.Sys.aum \
	src/Fetch.aum

STDLIB := \
	../austral/standard/src/Tuples.aui,../austral/standard/src/Tuples.aum \
	../austral/standard/src/Bounded.aui,../austral/standard/src/Bounded.aum \
	../austral/standard/src/Equality.aui,../austral/standard/src/Equality.aum \
	../austral/standard/src/Order.aui,../austral/standard/src/Order.aum \
	../austral/standard/src/Box.aui,../austral/standard/src/Box.aum \
	../austral/standard/src/Buffer.aui,../austral/standard/src/Buffer.aum \
	../austral/standard/src/String.aui,../austral/standard/src/String.aum \
	../austral/standard/src/StringBuilder.aui,../austral/standard/src/StringBuilder.aum \
	../austral/standard/src/IO/IO.aui,../austral/standard/src/IO/IO.aum \
	../austral/standard/src/IO/Terminal.aui,../austral/standard/src/IO/Terminal.aum

.DEFAULT: all
all: $(OUT)

$(OUT): $(OUT_C)
	$(CC) $(CFLAGS) $(OUT_C) -lm -o $(OUT)

$(OUT_C): $(SRC_FILES)
	$(AUSTRAL_BIN) compile $(STDLIB) $(SRC) --target-type=c --entrypoint=$(ENTRY) --output=$(OUT_C)

.PHONY: tc
tc:
	$(AUSTRAL_BIN) compile $(STDLIB) $(SRC) --target-type=tc

.PHONY: clean
clean:
	rm -f $(OUT) $(OUT_C)
