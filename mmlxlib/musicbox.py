#!/usr/bin/env python

# Copyright 2012 Craig Campbell
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import re, os, subprocess, sys, getopt, shutil
from warpwhistle import WarpWhistle
from util import Util
from instrument import Instrument
from listener import Listener
from logger import Logger

class MusicBox(object):
    VERSION = '1.0.2'

    def processArgs(self, args, local):
        options = {
            'verbose': False,
            'open_nsf': False,
            'listen': False,
            'local': local,
            'create_nsf': True,
            'create_mml': False,
            'separate_voices': False,
            'start': None,
            'end': None
        }

        if '--help' in args:
            return self.showUsage()

        try:
            for key,arg in enumerate(args):
                if arg == '--verbose':
                    options['verbose'] = True
                elif arg == '--open-nsf':
                    options['open_nsf'] = True
                elif arg == '--bob-omb':
                    options['separate_voices'] = True
                elif arg == '--create-nsf':
                    value = args[key + 1]
                    del(args[key + 1])
                    options['create_nsf'] = False if value == '0' else True
                elif arg == '--create-mml':
                    value = args[key + 1]
                    del(args[key + 1])
                    options['create_mml'] = True if value == '1' else False
                elif arg == '--watch':
                    options['listen'] = True
                    value = args[key + 1]
                    del(args[key + 1])
                    bits = value.split(':')
                    options['start'] = bits[0]
                    options['end'] = bits[1] if len(bits) > 1 else None
                else:
                    if options['start'] is None:
                        options['start'] = arg
                    elif options['end'] is None:
                        options['end'] = arg
        except:
            self.showUsage()

        if not options['create_nsf'] and not options['create_mml']:
            self.showUsage('You need to create an MML file or an NSF file')

        if options['start'] is None:
            self.showUsage('You haven\'t specified a file or directory to convert')

        if not Util.isFileOrDirectory(options['start']):
            self.showUsage(self.logger.color(options['start'], self.logger.YELLOW) + " is not a file or directory")

        if options['end'] is None:
            options['end'] = options['start'].replace('.mmlx', '.mml')

        options['end'] = options['end'].replace('.nsf', '.mml')

        return options

    def play(self, args, local):
        # create an intial logger so we can log before args are processed
        self.logger = Logger({"verbose": False})
        self.drawLogo()

        options = self.processArgs(args, local)
        self.options = options

        # set up the logger with the correct options
        self.logger = Logger(self.options)

        listener = Listener(self.logger)
        listener.onChange(self.processFile)

        if os.path.isdir(options['start']) and not os.path.isdir(options['end']):
            os.mkdir(options['end'])

        if self.options['listen']:
            listener.watch(options['start'], options['end'])
        else:
            listener.process(options['start'], options['end'])
            self.logger.log(self.logger.color('Done!', self.logger.PINK))
            sys.exit(0)

    def drawLogo(self):
        self.logger.log(self.logger.color('_|      _|  _|      _|  _|        _|      _|  ', self.logger.PINK))
        self.logger.log(self.logger.color('_|_|  _|_|  _|_|  _|_|  _|          _|  _|    ', self.logger.PINK))
        self.logger.log(self.logger.color('_|  _|  _|  _|  _|  _|  _|            _|      ', self.logger.PINK))
        self.logger.log(self.logger.color('_|      _|  _|      _|  _|          _|  _|    ', self.logger.PINK))
        self.logger.log(self.logger.color('_|      _|  _|      _|  _|_|_|_|  _|      _|  ', self.logger.PINK))
        self.logger.log(self.logger.color('                                 version ' + self.logger.color(MusicBox.VERSION, self.logger.PINK, True) + '\n', self.logger.PINK))

    def showUsage(self, message = None):
        logger = self.logger
        if message is not None:
            logger.log(logger.color('ERROR:', logger.RED))
            logger.log(message + "\n")

        logger.log(logger.color('ARGUMENTS:', logger.WHITE, True))
        logger.log(logger.color('--help', logger.WHITE) + '                                shows help dialogue')
        logger.log(logger.color('--verbose', logger.WHITE) + '                             shows verbose output')
        logger.log(logger.color('--open-nsf', logger.WHITE) + '                            opens nsf file on save')
        logger.log(logger.color('--bob-omb', logger.WHITE) + '                             generates a separate NSF file for each voice')
        logger.log(logger.color('--create-mml ' + logger.color('0', logger.YELLOW), logger.WHITE) +'                        creates an MML file on save (defaults to 0)')
        logger.log(logger.color('--create-nsf ' + logger.color('1', logger.YELLOW), logger.WHITE) +'                        creates an NSF file on save (defaults to 1)')
        logger.log(logger.color('--watch', logger.WHITE) + logger.color(' path/to/mmlx', logger.YELLOW) + logger.color(':', logger.GRAY) + logger.color('path/to/mml', logger.YELLOW) + '      watches for changes in first directory and compiles to second')
        logger.log(logger.color('\nEXAMPLES:', logger.WHITE, True))

        logger.log(logger.color('watch directory for changes in .mmlx files:', logger.GRAY))
        logger.log(logger.color('mmlx --watch path/to/mmlx', logger.BLUE, True))

        logger.log(logger.color('\nkeep compiled files in mml directory and open .nsf file on save:', logger.GRAY))
        logger.log(logger.color('mmlx --watch path/to/mmlx:path/to/mml --open-nsf', logger.BLUE, True))

        logger.log(logger.color('\nwatch a single file for changes:', logger.GRAY))
        logger.log(logger.color('mmlx --watch path/to/file.mmlx:path/to/otherfile.mml', logger.BLUE, True))

        logger.log(logger.color('\nrun once for a single file:', logger.GRAY))
        logger.log(logger.color('mmlx path/to/file.mmlx path/to/otherfile.nsf', logger.BLUE, True))
        logger.log(logger.color('mmlx path/to/file.mmlx', logger.BLUE, True))

        logger.log(logger.color('\nrun once for a directory:', logger.GRAY))
        logger.log(logger.color('mmlx path/to/mmlx path/to/nsf', logger.BLUE, True))
        logger.log('')

        sys.exit(1)

    def createNSF(self, path, open_file = False):
        self.logger.log('generating file: ' + self.logger.color(path.replace('.mml', '.nsf'), self.logger.YELLOW))

        nes_include_path = os.path.join(os.path.dirname(__file__), 'nes_include')
        bin_dir = os.path.join(nes_include_path, '../../bin')

        # copy the nes include directory locally so we don't have to deal with permission problems
        if not self.options['local']:
            new_nes_include_path = os.path.join(os.path.dirname(path), 'nes_include')
            shutil.copytree(nes_include_path, new_nes_include_path)
            nes_include_path = new_nes_include_path

        os.environ['NES_INCLUDE'] = nes_include_path
        command = os.path.join(bin_dir, 'ppmckc') if self.options['local'] else 'ppmckc'
        output = subprocess.Popen([command, '-m1', '-i', path], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        parts = output.communicate()
        stdout = parts[0]
        stderr = parts[1]

        command = os.path.join(bin_dir, 'nesasm') if self.options['local'] else 'nesasm'
        output = subprocess.Popen([command, '-s', '-raw', os.path.join(nes_include_path, 'ppmck.asm')], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        parts = output.communicate()
        stdout = parts[0]
        stderr = parts[1]

        nsf_path = os.path.join(nes_include_path, 'ppmck.nes')
        if not os.path.isfile(nsf_path):
            os.unlink('define.inc')
            self.logger.log('')
            if not self.options['local']:
                shutil.rmtree(nes_include_path)
            raise Exception('failed to create NSF file! Your MML is probably invalid.')

        os.rename(nsf_path, path.replace('.mml', '.nsf'))
        os.unlink('define.inc')
        os.unlink('effect.h')
        os.unlink(path.replace('.mml', '.h'))
        if not self.options['local']:
            shutil.rmtree(nes_include_path)

        if open_file and self.options['open_nsf']:
            subprocess.call(['open', path.replace('.mml', '.nsf')])

    def processFile(self, input, output, open_file = False):
        Instrument.reset()

        self.logger.log('processing file: ' + self.logger.color(input, self.logger.YELLOW), True)
        original_content = content = Util.openFile(input)

        whistle = WarpWhistle(content, self.logger, self.options)
        whistle.import_directory = os.path.dirname(input)

        while whistle.isPlaying():
            open_file = open_file and whistle.first_run

            song = whistle.play()

            new_output = output
            if song[1] is not None:
                new_output = new_output.replace('.mml', '_' + song[1] + '.mml')

            self.handleProcessedFile(song[0], new_output, open_file)

        if self.options['separate_voices']:
            self.logger.log("")

    def handleProcessedFile(self, content, output, open_file = False):
        if self.options['create_mml']:
            self.logger.log('generating file: ' + self.logger.color(output, self.logger.YELLOW))

        Util.writeFile(output, content)

        if self.options['create_nsf']:
            self.createNSF(output, open_file)

        if not self.options['create_mml']:
            Util.removeFile(output)

        if open_file:
            self.logger.log("")
