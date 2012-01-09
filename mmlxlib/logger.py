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
