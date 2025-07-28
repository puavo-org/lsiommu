# lsiommu

`lsiommu` is a command-line tool for Linux that lists IOMMU groups and PCI
devices. The output is by default plain text but can be optionally set to
JSON. `lsiommu` can be compile-time selected to discover devices either
from udev (the default) or sysfs.

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
