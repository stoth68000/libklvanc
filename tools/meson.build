sources = files(
  'klvanc_util.c',
  'demo.c',
  'parse.c',
  'smpte2038.c',
  'scte104.c',
  'genscte104.c',
  'gensmpte2038.c',
  'eia708.c',
  'smpte12_2.c',
  'afd.c',
  'udp.c',
  'url.c',
  'ts_packetizer.c',
  'klringbuffer.c',
  'pes_extractor.c',
)

thread_dep = dependency('threads')

foreach exe_name: [
  'klvanc_util',
  'klvanc_parse',
  'klvanc_smpte2038',
  'klvanc_scte104',
  'klvanc_genscte104',
  'klvanc_gensmpte2038',
  'klvanc_eia708',
  'klvanc_smpte12_2',
  'klvanc_afd',
]
  exe = executable(exe_name,
    sources,
    install: true,
    install_tag: 'bin',
    dependencies: [libklvanc_dep, thread_dep]
  )
  if exe_name in [
    'klvanc_eia708',
    'klvanc_genscte104',
    'klvanc_scte104',
    'klvanc_smpte12_2',
    'klvanc_gensmpte2038',
    'klvanc_afd']
    test_name = 'test_' + exe_name
    test(test_name, exe)
  elif exe_name == 'klvanc_smpte2038'
    test_name = 'test_' + exe_name
    test(test_name, exe,
      args : [
        '-i ../samples/smpte2038-sample-pid-01e9.ts',
        '-P 0x1e9'
      ]
    )
  endif
endforeach
