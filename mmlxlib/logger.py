
class Logger(object):
    BLUE = 'blue'
    LIGHT_BLUE = 'light_blue'
    PINK = 'pink'
    YELLOW = 'yellow'
    WHITE = 'white'
    GREEN = 'green'
    RED = 'red'
    GRAY = 'gray'
    UNDERLINE = 'underline'
    ITALIC = 'italic'

    def __init__(self, options):
        self.verbose = options["verbose"]

    def color(self, message, color, bold = False):
        colors = {
            'italic': '\033[3m',
            'underline': '\033[4m',
            'light_blue': '\033[96m',
            'pink': '\033[95m',
            'blue': '\033[94m',
            'yellow': '\033[93m',
            'green': '\033[92m',
            'red': '\033[91m',
            'gray': '\033[90m',
            'white': '\033[0m'
        }

        end = '\033[0m'

        if not color in colors:
            return message

        bold_start = '\033[1m' if bold else ''

        return colors[color] + bold_start + message + end

    def log(self, message, verbose_only=False):
        if verbose_only and not self.verbose:
            return

        print message
