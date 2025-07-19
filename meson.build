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
  'iommu.c',
  'main.c',
  'radix.c',
  'strbuf.c',
  'udev.c',
]

deps = [
  dependency('argtable2'),
  dependency('libudev'),
]

exe = executable(
  'lsiommu',
  sources,
  dependencies: deps,
  install: true,
)

install_man('lsiommu.1')

test('basic test', exe)
test('help test', exe, args: ['--help'])
