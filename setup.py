#!/usr/bin/env python

from distutils.core import setup

setup(name='mmlx',
    version='1.0',
    description='Extended version of music macro language which generates MML and NSF files for Nintendo music composing',
    author='Craig Campbell',
    author_email='iamcraigcampbell@gmail.com',
    packages=['mmlxlib'],
    package_data={'mmlxlib': ['nes_include/ppmck.asm', 'nes_include/ppmck/*']},
    scripts=['bin/mmlx', 'bin/ppmckc', 'bin/nesasm']
)