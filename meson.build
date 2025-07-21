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
  'iommu/main.c',
]

extra_cflags = []
optional_deps = []
discovery = get_option('discovery')
if discovery == 'udev'
  sources += 'iommu/udev.c'
  optional_deps += dependency('libudev')
  extra_cflags += '-DCONFIG_LIBUDEV'
elif discovery == 'sysfs'
  sources += 'iommu/sysfs.c'
else
  error('Unsupported discovery method')
endif

extra_cflags += '-DCONFIG_DISCOVERY=' + discovery

deps = [
  dependency('argtable2'),
  dependency('libcjson'),
]

exe = executable(
  'lsiommu',
  sources,
  c_args: extra_cflags,
  dependencies: deps + optional_deps,
  install: true,
)

install_man('lsiommu.1')

test('basic test', exe)
test('help test', exe, args: ['--help'])
