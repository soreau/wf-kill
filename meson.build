project(
	'wf-kill',
	'c',
	'cpp',
	version: '0.1.0',
	license: 'MIT',
	meson_version: '>=0.51.0',
	default_options: [
		'cpp_std=c++17',
		'c_std=c11',
	],
)

wayfire = dependency('wayfire')
wfconfig = dependency('wf-config')

add_project_arguments(['-DWLR_USE_UNSTABLE'], language: ['cpp', 'c'])

subdir('metadata')
subdir('proto')
subdir('src')
