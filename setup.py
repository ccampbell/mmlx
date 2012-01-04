#!/usr/bin/env python

from distutils.core import setup

setup(name='xmml',
    version='1.0',
    description='Extended version of music macro language which generates MML and NSF files for Nintendo music composing',
    author='Craig Campbell',
    author_email='iamcraigcampbell@gmail.com',
    packages=['xmmllib'],
    package_data={'xmmllib': ['nes_include/ppmck.asm', 'nes_include/ppmck/*']},
    scripts=['bin/xmml', 'bin/ppmckc', 'bin/nesasm']
)