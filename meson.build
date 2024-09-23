project(
  'libenc',
  ['c', 'cpp'],
  version: '0.1.0',
  license: 'MIT',
  meson_version: '>=0.58.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    ],
  )

libva = dependency('libva')
libva_drm = dependency('libva-drm')

files_libenc = files(
  'src/enc.cpp',
  'src/dev.cpp',
  'src/task.cpp',
  'src/surface.cpp',
  'src/bitstream.cpp',
  'src/bitstream_av1.cpp',
  'src/bitstream_h264.cpp',
  'src/bitstream_hevc.cpp',
  'src/encoder.cpp',
  'src/encoder_av1.cpp',
  'src/encoder_h264.cpp',
  'src/encoder_hevc.cpp',
)

inc_libenc = include_directories('include')

libenc = static_library(
  'enc',
  files_libenc,
  dependencies: [libva, libva_drm],
  include_directories: inc_libenc,
  install: true,
)

libenc_dep = declare_dependency(
  link_with: libenc,
  include_directories: inc_libenc,
)

if get_option('test-app').enabled()
  subdir('app')
endif
