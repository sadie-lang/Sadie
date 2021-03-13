BUILD_DIR := build

default: sadie

clean:
	@ rm -rf $(BUILD_DIR)
	@ rm -rf gen

debug:
	@ $(MAKE) -f util/c.make NAME=sadie_debug MODE=debug SOURCE_DIR=c

sadie:
	@ $(MAKE) -f util/c.make NAME=sadiec MODE=release SOURCE_DIR=c
	@ cp build/sadiec sadiec

.PHONY: clean sadie debug
