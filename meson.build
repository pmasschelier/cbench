project('cbench', 'c',
  version: '0.1',
  meson_version: '>=1.1.0',
  license: 'MIT',
  license_files: 'LICENSE',
  default_options: [ 'warning_level=3' ]
)

project_dep = declare_dependency(include_directories: 'include')

install_headers(['include/measurement.h'], subdir: meson.project_name())
pkgconfig = import('pkgconfig')
pkgconfig.generate(
  name : meson.project_name(),
  filebase : meson.project_name(),
  description : 'A header-only C benchmarking tool',
  subdirs : meson.project_name(),
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

if not meson.is_subproject()
  executable('eval', 'src/eval.c', dependencies: [project_dep, m_dep])
endif
