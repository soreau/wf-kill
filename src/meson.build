sources = ['main.cpp', 'plugin/wayfire-kill-view.cpp']

wf_kill = shared_module('wf-kill', sources,
    dependencies: [wayfire, wf_server_protos],
    install: true, install_dir: join_paths(get_option('libdir'), 'wayfire'))
    
subdir('client')
