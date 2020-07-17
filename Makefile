BUILD_DIR := build

clean:
	@ rm -rf $(BUILD_DIR)

sadie:
	@ cc src/*.c src/includes/*.h src/includes/gb/gb.h src/parser/*.c -o sadie -pthread -ldl

.PHONY: sadie clean