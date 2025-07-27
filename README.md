# lsiommu

A command-line utility for Linux, which lists IOMMU groups and the associated
PCI devices.

## Dependencies

- a C11-capable compiler.
- make
- pkg-config
- libudev but only when built with udev discovery.

## Building

- `make` or `make DISCOVERY=udev` builds a udev backed version.
- `make DISCOVERY=sysfs` builds a sysfs backed version.

## License

This project is licensed under the **GNU General Public License v3.0**. Read
[LICENSE](LICENSE) for the full license text.
