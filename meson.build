project(
  'lsiommu',
  'c',
  version: '1.0.0',
  license: 'GPL-3.0-or-later',
  default_options: [
    'c_std=c11',
    'warning_level=3',
    'werror=true',
  ],
  meson_version: '>= 1.0.1'
)

add_project_arguments(
  '-Wpedantic',
  '-Wformat=2',
  '-Wno-unused-variable',
  language: 'c'
)

sources = [
  'down.c',
  'main.c',
  'pci.c',
  'radix.c',
  'strbuf.c',
  'sysfs.c',
  'iommu/json.c',
  'iommu/main.c',
]

extra_cflags = []
deps = []
discovery = get_option('discovery')
if discovery == 'udev'
  sources += 'iommu/udev.c'
  deps += dependency('libudev')
  extra_cflags += '-DCONFIG_LIBUDEV'
elif discovery == 'sysfs'
  sources += 'iommu/sysfs.c'
else
  error('Unsupported discovery method')
endif

extra_cflags += '-DCONFIG_DISCOVERY=' + discovery

exe = executable(
  'lsiommu',
  sources,
  c_args: extra_cflags,
  dependencies: deps + deps,
  install: true,
)

install_man('lsiommu.1')
