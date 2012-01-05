import re, os, subprocess
from warpwhistle import WarpWhistle
from util import Util
from instrument import Instrument

class MusicBox(object):

    def __init__(self, logger=None):
        self.logger = logger

    def createNSF(self, path):

        nes_include_path = os.path.join(os.path.dirname(__file__), 'nes_include')

        os.environ['NES_INCLUDE'] = nes_include_path
        subprocess.call(['ppmckc', '-m1', '-i', path])

        subprocess.call(['nesasm', '-s', '-raw', os.path.join(nes_include_path, 'ppmck.asm')])

        os.rename(os.path.join(nes_include_path, 'ppmck.nes'), path.replace('.mml', '.nsf'))
        os.unlink('define.inc')
        os.unlink('effect.h')
        os.unlink(path.replace('.mml', '.h'))

    def processFile(self, input, output):
        Instrument.reset()

        original_content = content = Util.openFile(input)

        whistle = WarpWhistle(content, self.logger)
        whistle.import_directory = os.path.dirname(input)
        content = whistle.play()

        self.logger.log('saving file as ' + output)
        Util.writeFile(output, content)

        self.createNSF(output)

