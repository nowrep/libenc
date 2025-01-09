libvpl = dependency('vpl')
libdrm = dependency('libdrm')

files_libmfx = files(
  'mfx.cpp',
  'caps.cpp',
  'session.cpp',
  'surface.cpp',
)

libmfx = library(
  'mfx',
  files_libmfx,
  dependencies: [libvpl, libdrm, libenc_dep],
)
