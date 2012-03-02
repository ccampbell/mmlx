#!/usr/bin/env python

from distutils.core import setup

setup(
    name = 'mmlx',
    version = '1.0.2',
    description = 'Nintendo NES chiptune programming language',
    long_description = 'MMLX stands for MML eXtended.  It is an extended version of Music Macro Language that generates MML and NSF files for Nintendo NES music composing',
    author = 'Craig Campbell',
    author_email = 'iamcraigcampbell@gmail.com',
    url = 'https://github.com/ccampbell/mmlx',
    download_url = 'https://github.com/ccampbell/mmlx/zipball/1.0.2',
    license = 'Apache Software License',
    packages = ['mmlxlib'],
    package_data = {'mmlxlib': ['nes_include/ppmck.asm', 'nes_include/ppmck/*']},
    scripts = ['bin/mmlx', 'bin/ppmckc', 'bin/nesasm']
)
