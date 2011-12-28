import re, os
from warpwhistle import WarpWhistle
from util import Util
from instrument import Instrument

class MusicBox(object):

    def __init__(self, logger=None):
        self.logger = logger

    def processFile(self, input, output):
        Instrument.reset()

        original_content = content = Util.openFile(input)

        whistle = WarpWhistle(content, self.logger)
        whistle.import_directory = os.path.dirname(input)
        content = whistle.play()

        # print content

        self.logger.log('saving file as ' + output)
        Util.writeFile(output, content)

