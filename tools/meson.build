sources = files(
  'klvanc_util.c',
  'demo.c',
  'parse.c',
  'scte104.c',
  'genscte104.c',
  'gensmpte2038.c',
  'eia708.c',
  'smpte12_2.c',
  'smpte12_3.c',
  'smpte299.c',
  'afd.c',
  'sdp.c',
  'ts_packetizer.c',
)

is_windows = host_machine.system() == 'windows'
tool_c_args = []
if is_windows
  tool_c_args += [
    '-DKLVANC_DISABLE_SMPTE2038_TOOL',
    '-DKLVANC_DISABLE_TEST_API_TOOL',
  ]
else
  sources += files(
    'smpte2038.c',
    'udp.c',
    'url.c',
    'klringbuffer.c',
    'pes_extractor.c',
    'test_api.c',
  )
endif

thread_dep = dependency('threads')

exe_names = [
  'klvanc_util',
  'klvanc_parse',
  'klvanc_scte104',
  'klvanc_genscte104',
  'klvanc_gensmpte2038',
  'klvanc_eia708',
  'klvanc_smpte12_2',
  'klvanc_smpte12_3',
  'klvanc_smpte299',
  'klvanc_afd',
  'klvanc_sdp',
]

if not is_windows
  exe_names += ['klvanc_smpte2038', 'klvanc_test_api']
endif

foreach exe_name: exe_names
  exe = executable(exe_name,
    sources,
    c_args: tool_c_args,
    install: true,
    install_tag: 'bin',
    dependencies: [libklvanc_dep, thread_dep]
  )
  if exe_name in [
    'klvanc_eia708',
    'klvanc_genscte104',
    'klvanc_scte104',
    'klvanc_smpte12_2',
    'klvanc_smpte12_3',
    'klvanc_smpte299',
    'klvanc_gensmpte2038',
    'klvanc_afd',
    'klvanc_sdp']
    test_name = 'test_' + exe_name
    test(test_name, exe)
  elif not is_windows and exe_name == 'klvanc_smpte2038'
    test_name = 'test_' + exe_name
    test(test_name, exe,
      args : [
        '-i ../samples/smpte2038-sample-pid-01e9.ts',
        '-P 0x1e9'
      ]
    )
  elif exe_name == 'klvanc_test_api'
    test('test_' + exe_name, exe)
  endif
endforeach
