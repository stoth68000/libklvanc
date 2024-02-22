klvanc_sources = files(
  'core.c',
  'core-packet-eia_708b.c',
  'core-packet-eia_608.c',
  'core-packet-scte_104.c',
  'core-packet-afd.c',
  'core-packet-sdp.c',
  'core-packet-smpte_12_2.c',
  'core-packet-smpte_2108_1.c',
  'core-packets.c',
  'core-lines.c',
  'core-did.c',
  'core-pixels.c',
  'core-checksum.c',
  'smpte2038.c',
  'core-cache.c',
  'core-packet-kl_u64le_counter.c',
)

klvanc_headers = files(
  'libklvanc/vanc.h',
  'libklvanc/did.h',
  'libklvanc/pixels.h',
  'libklvanc/smpte2038.h',
  'libklvanc/vanc-eia_708b.h',
  'libklvanc/vanc-eia_608.h',
  'libklvanc/vanc-scte_104.h',
  'libklvanc/vanc-smpte_12_2.h',
  'libklvanc/vanc-smpte_2108_1.h',
  'libklvanc/vanc-packets.h',
  'libklvanc/vanc-lines.h',
  'libklvanc/vanc-afd.h',
  'libklvanc/vanc-sdp.h',
  'libklvanc/vanc-checksum.h',
  'libklvanc/klrestricted_code_path.h',
  'libklvanc/cache.h',
  'libklvanc/vanc-kl_u64le_counter.h',
)

install_headers(klvanc_headers, subdir : 'libklvanc')

klvanc_incdirs = include_directories('.')

thread_dep = dependency('threads')

libklvanc = library('klvanc', klvanc_sources,
  include_directories : klvanc_incdirs,
  install : true,
  dependencies: [thread_dep],
)

libklvanc_dep = declare_dependency(link_with : libklvanc,
  include_directories : klvanc_incdirs,
)
