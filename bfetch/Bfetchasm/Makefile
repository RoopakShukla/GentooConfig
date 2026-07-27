TARGET ?= bfetchasm
SRC := fetch.s
OBJ := $(SRC:.s=.o)

AS ?= as
LD ?= ld
ASFLAGS ?= --64
LDFLAGS ?= -static -s

$(TARGET): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: clean
