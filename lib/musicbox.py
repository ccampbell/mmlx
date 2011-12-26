import re
from warpwhistle import WarpWhistle

class MusicBox(object):

    def __init__(self, logger=None):
        self.logger = logger

    def processFile(self, input, output):
        file = open(input, "r")
        original_contents = contents = file.read()
        file.close()

        whistle = WarpWhistle(contents, self.logger)
        contents = whistle.play()

        print contents
        # contents = self.processVoice('A', contents)
        # contents = self.processVoice('B', contents)
        # contents = self.processVoice('C', contents)
        # contents = self.processVoice('D', contents)
        # contents = self.processVoice('E', contents)

        file = open(output, "w")
        file.write(contents)
        file.close()

