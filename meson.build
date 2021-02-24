# meson.build for 9menu
# license: public domain
# Contributed by Anderson Torres, February 2021
project ('9menu', 'c')

executable ('9menu', '9menu.c',
            install: true,
            dependencies: [ dependency('x11'),
                            dependency('xext') ])
install_man ('9menu.1')
