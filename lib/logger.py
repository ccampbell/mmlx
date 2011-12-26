
class Logger(object):

    def __init__(self, verbose=False):
        self.verbose = verbose

    def log(self, message, verbose_only=False):
        if verbose_only and not self.verbose:
            return

        print message
