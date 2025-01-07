libvpl = dependency('vpl')

files_libmfx = files(
  'mfx.cpp',
)

libmfx = library(
  'mfx',
  files_libmfx,
  dependencies: [libvpl, libenc_dep],
)
