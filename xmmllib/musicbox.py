import re, os, subprocess
from warpwhistle import WarpWhistle
from util import Util
from instrument import Instrument

class MusicBox(object):

    def __init__(self, logger=None):
        self.logger = logger

    def createNSF(self, path):
        pass
        # p = subprocess.Popen(["which", "xmml"], stdout=subprocess.PIPE)
        # # print p.communicate()
        # print os.path.dirname(os.path.realpath(p.communicate()[0].strip()))
        # # print os.path.realpath(out)
        # return
        #
        # os.chdir(os.path.dirname(path))
        # # os.environ['NES_INCLUDE'] =
        # subprocess.call(['ppmckc', '-m1', '-i', os.path.basename(path)])
        # subprocess.call(['nesasm', '-s', '-raw', ])

    def processFile(self, input, output):
        Instrument.reset()

        original_content = content = Util.openFile(input)

        whistle = WarpWhistle(content, self.logger)
        whistle.import_directory = os.path.dirname(input)
        content = whistle.play()

        self.logger.log('saving file as ' + output)
        Util.writeFile(output, content)

        self.createNSF(output)

