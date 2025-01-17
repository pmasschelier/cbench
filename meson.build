project('cbench', 'c',
  version: '0.1',
  meson_version: '>=1.1.0',
  license: 'MIT',
  license_files: 'LICENSE',
  default_options: [ 'warning_level=3' ]
)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

project_dep = declare_dependency(include_directories: 'include', dependencies: [m_dep])
set_variable(meson.project_name() + '_dep', project_dep)

install_headers(['include/measurement.h'], subdir: meson.project_name())
pkgconfig = import('pkgconfig')
pkgconfig.generate(
  name : meson.project_name(),
  filebase : meson.project_name(),
  description : 'A header-only C benchmarking tool',
  subdirs : meson.project_name(),
)

if not meson.is_subproject()
  executable('eval', 'src/eval.c', dependencies: [project_dep])
endif
