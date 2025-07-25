BUILD_DIR := build
DISCOVERY ?= udev

.PHONY: all test install clean

all: $(BUILD_DIR)/lsiommu

$(BUILD_DIR)/lsiommu:
	@meson setup --buildtype debug $(BUILD_DIR) -Ddiscovery=$(DISCOVERY)
	@meson compile -C $(BUILD_DIR)

test: all
	@meson test -C $(BUILD_DIR)

install: all
	@meson install -C $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)
