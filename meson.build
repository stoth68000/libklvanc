project('libklvanc', 'c',
  default_options : ['buildtype=debugoptimized'])

subdir('src')
subdir('tools')

doxygen = find_program('doxygen', required : false)

datadir = join_paths(get_option('datadir'), 'doc', 'libklvanc')
doxyfile = files('doxygen/libklvanc.doxyconf')

if doxygen.found()
  html_target = custom_target('libklvanc-docs',
                            build_by_default: false,
                            input: doxyfile,
                            output: 'html',
                            command: [doxygen, '@INPUT@'],
                            install: true,
                            install_dir: datadir)
endif
