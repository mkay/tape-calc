project('tape-calc', 'cpp', 'c',
  version: '1.0',
  default_options: ['cpp_std=c++20', 'buildtype=debugoptimized', 'optimization=2']
)

core_sources = [
  'src/main.cpp',
  'src/mainwindow.cpp',
  'src/calculator_engine.cpp',
]

gtkmm = dependency('gtkmm-4.0', required: true)
gtk4 = dependency('gtk4', required: true)

executable('tape-calc', core_sources,
  dependencies: [ gtkmm, gtk4],
)
