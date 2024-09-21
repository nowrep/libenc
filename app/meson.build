avutil = dependency('libavutil')
avcodec = dependency('libavcodec')
avformat = dependency('libavformat')

executable(
  'encapp',
  ['main.c'],
  dependencies: [libenc_dep, avutil, avcodec, avformat],
)
