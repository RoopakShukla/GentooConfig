# This workload is startup-heavy; smaller static code beats larger -O3 codegen here.
# Optional experimental path: make CFLAGS_EXTRA='-DBFETCH_ENABLE_BTRFS_PACMAN'
ifeq ($(origin CC), default)
  ifneq ($(shell command -v zig 2>/dev/null),)
    CC := zig cc
    CFLAGS ?= -Oz -ffunction-sections -fdata-sections -target native-linux-musl -static -std=c11 \
              -fomit-frame-pointer -fno-plt -pipe -fno-stack-protector \
              -fno-unwind-tables -fno-asynchronous-unwind-tables -Wall -Wextra
    LDFLAGS ?= -s -Wl,--build-id=none -Wl,--gc-sections
  else
    CC := gcc
    CFLAGS ?= -Os -ffunction-sections -fdata-sections -march=native -std=c11 -flto \
              -fomit-frame-pointer -fno-plt -pipe -Wall -Wextra
    LDFLAGS ?= -flto -Wl,--gc-sections
  endif
else
  CFLAGS ?= -Os -ffunction-sections -fdata-sections -march=native -std=c11 -flto \
             -fomit-frame-pointer -fno-plt -pipe -Wall -Wextra
  LDFLAGS ?= -flto -Wl,--gc-sections
endif
TARGET  ?= fetch
SRC     := fetch.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(CFLAGS_EXTRA) $(LDFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET) $(TARGET).old *.gcda *.gcno default.profdata

# PGO (run training workload between the two builds), e.g.:
#   make clean && make CFLAGS_EXTRA='-fprofile-generate' && for i in 1 2 3 4 5; do ./fetch >/dev/null; done
#   make clean && make CFLAGS_EXTRA='-fprofile-use'

.PHONY: clean
