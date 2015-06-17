{
    'targets':
    [
        {
            'target_name': "node-sdl2_mixer",
            'include_dirs':
            [   
                "<(module_root_dir)",
                "<!(node -e \"require('node-sdl2/include')\")",
                "<!(node -e \"require('nan')\")"
            ],
            'sources':
            [
                "node-sdl2_mixer.cc"
            ],
            'conditions':
            [
                [
                    'OS=="linux"',
                    {
                        'include_dirs': [ "/usr/local/include/SDL2", "/usr/local/include/SDL2_mixer" ],
                        'cflags': [ "<!@(sdl2-config --cflags)" ],
                        'ldflags': [ "<!@(sdl2-config --libs)" ],
                        'libraries': [ "-lSDL2", "-lSDL2_mixer" ]
                    }
                ],
                [
                    'OS=="mac"',
                    {
                        'include_dirs': [ "/usr/local/include/SDL2", "/usr/local/include/SDL2_mixer" ],
                        'cflags': [ "<!@(sdl2-config --cflags)" ],
                        'ldflags': [ "<!@(sdl2-config --libs)" ],
                        'libraries': [ "-L/usr/local/lib", "-lSDL2", "-lSDL2_mixer" ]
                    }
                ],
                [
                    'OS=="win"',
                    {
                        'include_dirs': [ "$(SDL2_ROOT)/include", "$(SDL2_MIXER_ROOT)/include" ],
                        'libraries':
                        [
                            "$(SDL2_ROOT)/lib/x64/SDL2.lib",
                            "$(SDL2_MIXER_ROOT)/lib/x64/SDL2_mixer.lib"
                        ],
                        'copies':
                        [
                            {
                                'destination': "${USERPROFILE}/bin",
                                'files':
                                [
                                    "$(SDL2_MIXER_ROOT)/lib/x64/SDL2_mixer.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/libFLAC-8.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/libmikmod-2.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/libmodplug-1.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/libogg-0.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/libvorbis-0.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/libvorbisfile-3.dll",
                                    "$(SDL2_MIXER_ROOT)/lib/x64/smpeg2.dll"
                                ]
                            }
                        ]
                    }
                ]
            ]
        }
    ]
}
