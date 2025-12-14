# ====== CONFIG ======
PORT := /dev/ttyUSB0

# ====== TARGETS ======

.PHONY: all default compile flash monitor

default: compile flash monitor
all: default

compile:
	pio run

flash:
	pio run --target upload --upload-port $(PORT)

monitor:
	pio device monitor --port $(PORT)

