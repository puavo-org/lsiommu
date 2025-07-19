BUILD_DIR := build

.PHONY: all test install clean

all: $(BUILD_DIR)/lsiommu

$(BUILD_DIR)/lsiommu:
	@if [ ! -d "$(BUILD_DIR)" ]; then	\
		meson setup --buildtype debug $(BUILD_DIR);	\
	fi
	@meson compile -C $(BUILD_DIR)

test: all
	@meson test -C $(BUILD_DIR)

install: all
	@meson install -C $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)
