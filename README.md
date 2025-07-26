# lsiommu

Lists all IOMMU groups and their associated PCI devices. 

## Dependencies

- argtable2
- libudev
- make
- meson
- ninja-build
- pkg-config

## Building

There are two approaches for doing builds.

Run `make` in order to create a *development build*. It takes care of
orchestrating Meson to create a debug build. A sysfs discovery backend,
instead of udev, can be selected as follows:  `make DISCOVERY=sysfs`.

Orchestrate `meson` manually in order to create *production builds*.
