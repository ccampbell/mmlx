import re, os, subprocess
from warpwhistle import WarpWhistle
from util import Util
from instrument import Instrument

class MusicBox(object):

    def __init__(self, logger = None, options = {}):
        self.logger = logger
        self.options = options
        self.drawLogo()

    def drawLogo(self):
        self.logger.log(self.logger.color('_|      _|  _|      _|  _|        _|      _|  ', self.logger.PINK))
        self.logger.log(self.logger.color('_|_|  _|_|  _|_|  _|_|  _|          _|  _|    ', self.logger.PINK))
        self.logger.log(self.logger.color('_|  _|  _|  _|  _|  _|  _|            _|      ', self.logger.PINK))
        self.logger.log(self.logger.color('_|      _|  _|      _|  _|          _|  _|    ', self.logger.PINK))
        self.logger.log(self.logger.color('_|      _|  _|      _|  _|_|_|_|  _|      _|  ', self.logger.PINK))
        self.logger.log(self.logger.color('                                 version ' + self.logger.color('1.0', self.logger.PINK, True) + '\n', self.logger.PINK))

    def showUsage(self, message = None):
        logger = self.logger
        if message is not None:
            logger.log(logger.color('ERROR:', logger.RED))
            logger.log(message)

        logger.log(logger.color('ARGUMENTS:', logger.WHITE, True))
        logger.log(logger.color('--help', logger.GRAY) + '                                shows help dialogue')
        logger.log(logger.color('--verbose', logger.GRAY) + '                             shows verbose output')
        logger.log(logger.color('--open-nsf', logger.GRAY) + '                            opens nsf file on save')
        logger.log(logger.color('--watch', logger.GRAY) + logger.color(' path/to/mmlx', logger.YELLOW) + logger.color(':', logger.GRAY) + logger.color('path/to/mml', logger.YELLOW) + '      watch for changes in first directory and compile to second')
        logger.log(logger.color('\nEXAMPLES:', logger.WHITE, True))

        logger.log(logger.color('watch directory for changes in .mmlx files:', logger.GRAY))
        logger.log(logger.color('mmlx --watch path/to/mmlx', logger.BLUE, True))

        logger.log(logger.color('\nkeep compiled files in mml directory and open .nsf file on save:', logger.GRAY))
        logger.log(logger.color('mmlx --watch path/to/mmlx:path/to/mml --open-nsf', logger.BLUE, True))

        logger.log(logger.color('\nwatch a single file for changes:', logger.GRAY))
        logger.log(logger.color('mmlx --watch path/to/file.mmlx:path/to/otherfile.mml', logger.BLUE, True))

        logger.log(logger.color('\nrun once for a single file:', logger.GRAY))
        logger.log(logger.color('mmlx path/to/file.mmlx path/to/otherfile.mml', logger.BLUE, True))
        logger.log(logger.color('mmlx path/to/file.mmlx', logger.BLUE, True))

        logger.log(logger.color('\nrun once for a directory:', logger.GRAY))
        logger.log(logger.color('mmlx path/to/mmlx path/to/mml', logger.BLUE, True))
        logger.log('')

    def createNSF(self, path, open_file = False):
        self.logger.log('generating file: ' + self.logger.color(path.replace('.mml', '.nsf'), self.logger.YELLOW))

        nes_include_path = os.path.join(os.path.dirname(__file__), 'nes_include')
        bin_dir = os.path.join(nes_include_path, '../../bin')

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
            raise Exception('failed to create NSF file! Your MML is probably invalid.')

        os.rename(nsf_path, path.replace('.mml', '.nsf'))
        os.unlink('define.inc')
        os.unlink('effect.h')
        os.unlink(path.replace('.mml', '.h'))

        if open_file and self.options['open_nsf']:
            subprocess.call(['open', path.replace('.mml', '.nsf')])

    def processFile(self, input, output, open_file = False):
        Instrument.reset()

        self.logger.log('processing file: ' + self.logger.color(input, self.logger.YELLOW), True)
        original_content = content = Util.openFile(input)

        whistle = WarpWhistle(content, self.logger, self.options)
        whistle.import_directory = os.path.dirname(input)
        content = whistle.play()

        self.logger.log('generating file: ' + self.logger.color(output, self.logger.YELLOW))
        Util.writeFile(output, content)

        if self.options['create_nsf']:
            self.createNSF(output, open_file)

        self.logger.log("")

