import re, os
from warpwhistle import WarpWhistle
from util import Util

class MusicBox(object):

    def __init__(self, logger=None):
        self.logger = logger

    def processFile(self, input, output):
        original_content = content = Util.openFile(input)

        whistle = WarpWhistle(content, self.logger)
        whistle.import_directory = os.path.dirname(input)
        content = whistle.play()

        print content

        Util.writeFile(output, content)

