project('sway-atas', 'c', version: 'git',  default_options : ['c_std=c99', 'buildtype=release', 'warning_level=2'])
cc = meson.get_compiler('c')
cc = meson.get_compiler('c')

pkgcfg = import('pkgconfig')

inc = include_directories('.')
gtk = dependency('gtk+-3.0')
wayland = dependency('wayland-client')
gio = dependency('gio-unix-2.0')
jsonc = dependency('json-c')

git = find_program('git', native: true, required: false)
tidy = find_program('clang-tidy', required: false)

version = meson.project_version()

if git.found()
    version = run_command(git, 'describe').stdout().strip()
endif

add_project_arguments('-D_GNU_SOURCE', '-DVERSION="' + version + '"', language : 'c')
add_project_link_arguments('-rdynamic', language : 'c')

sources = ['ipc-client.c',
           'log.c',
           'main.c',
           'sway-appswitch.c',
           'sway-client.c']

deps = [gtk, wayland, gio, jsonc]

pkgcfg.generate(name : meson.project_name(),
				description : 'Sway Application Switcher',
				requires : wayland)

if tidy.found()
    run_target(
        'tidy',
        command: [
            tidy,
            '-checks=*',
            '-p',
            meson.build_root()
        ] + sources
    )
endif

executable(meson.project_name(), sources, include_directories : inc, dependencies : deps, install : true)