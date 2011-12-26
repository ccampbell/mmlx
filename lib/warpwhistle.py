import re

class WarpWhistle(object):
    TEMPO = 'tempo'
    VOLUME = 'volume'
    TIMBRE = 'timbre'
    OCTAVE = None

    def __init__(self, content, logger):
        self.content = content
        self.logger = logger
        self.transpose = 0
        self.current_voices = []
        self.data = {}

    def getDataForVoice(self, voice, key):
        if not voice in self.data:
            return None

        if not key in self.data[voice]:
            return None

        return self.data[voice][key]

    def setDataForVoice(self, voice, key, value):
        if not voice in self.data:
            self.data[voice] = {}

        self.data[voice][key] = value

    def setDataForVoices(self, voices, key, value):
        for voice in voices:
            self.setDataForVoice(voice, key, value)

    def stripComments(self, content):
        # replace all /* comments */
        content = re.sub(re.compile(r'(/\*(.*)\*/)', re.MULTILINE | re.DOTALL), '', content)

        # replace all ; comments
        content = re.sub(re.compile(r'(;.*)$', re.MULTILINE), '', content)

        # replace empty lines
        content = re.sub(re.compile(r'\n{2,}', re.MULTILINE), '\n', content)

        return content

    def collapseSpaces(self, content):
        # collapse multiple spaces into a single space
        return re.sub(re.compile(r'\s{2,}', re.MULTILINE), ' ', content)

    def processVariables(self, content):
        matches = re.search(r'#TRANSPOSE\s{1,}((\+|\-)?(\d+))\n', content)
        num = '-' if matches.group(2) == '-' else ''
        num += matches.group(3)
        self.transpose = int(num)

        content = content.replace(matches.group(0), '')
        return content

    # def processVoice(self, voice, content):
    #     regex = '(' + voice + '\s{0,}(.*?)(?=(\n[A-Z^' + voice + ']{1,2}\s|\n?\Z)))'
    #     matches = re.findall(regex, content, re.MULTILINE | re.DOTALL)
    #     print matches
    #     return content

    def moveToOctave(self, new_octave, last_octave):
        diff = new_octave - last_octave
        ticks = abs(diff)
        symbol = '>' if diff > 0 else '<'
        return symbol * ticks

    def processWord(self, word, next_word, prev_word):
        if not word:
            return word

        # matches a voice declaration
        if re.match(r'[A-Z]{1,2}$', word):
            self.current_voices = list(word)
            return word

        # matches a tempo declaration
        if re.match(r't\d+$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.TEMPO, int(word[1:]))
            return word

        # volume change
        if re.match(r'@v\d+$', word) and next != '=':
            self.setDataForVoices(self.current_voices, WarpWhistle.VOLUME, int(word[2:]))
            return word

        # timbre change
        if re.match(r'@@\d+$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.TIMBRE, int(word[2:]))
            return word

        # explicit octave change (with o4 o3 etc)
        if re.match(r'o\d+$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.OCTAVE, int(word[1:]))
            return word

        # octave change with > or < or >>>
        if re.match(r'\>+|\<+', word):
            direction = word[0]
            count = len(word)
            current_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)
            self.setDataForVoices(self.current_voices, WarpWhistle.OCTAVE, current_octave + (count if direction == '>' else -count))
            return word

        # rewrite special voices for xmml such as C4 or G+/4^8
        match = re.match(r'(\[)?([A-G]{1})(\+|\-)?(\d{1,2})(\/(\d+)(\^[0-9\^]+)?)?(\]\d+)?', word)
        if match:
            new_word = ""

            current_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)

            octave = match.group(4)

            if current_octave is None:
                new_word += 'o' + octave + ' '
            elif int(octave) != current_octave:
                new_word += self.moveToOctave(int(octave), current_octave) + ' '

            self.setDataForVoices(self.current_voices, WarpWhistle.OCTAVE, int(octave))

            if match.group(1):
                new_word += '['

            # note
            new_word += match.group(2).lower()

            # accidental
            if match.group(3):
                new_word += match.group(3)

            if match.group(6):
                new_word += match.group(6)

            if match.group(7):
                new_word +=  match.group(7)

            if match.group(8):
                new_word += match.group(8)

            return new_word

        # print "PROCESS:",word
        # print "PREV:",prev_word
        # print "NEXT:",next_word
        # print ""
        return word

    def processLine(self, line):
        words = line.split(' ')
        new_words = []

        for key, word in enumerate(words):
            next_word = None
            prev_word = None

            if len(words) > key + 1:
                next_word = words[key + 1]

            if len(words) > key - 1:
                prev_word = words[key - 1]

            new_words.append(self.processWord(word, next_word, prev_word))

        return ' '.join(new_words)

    def process(self, content):
        self.logger.log('stripping comments', True)
        content = self.stripComments(content)

        self.logger.log('parsing variables', True)
        content = self.processVariables(content)

        self.logger.log('collapsing spaces', True)
        content = self.collapseSpaces(content)


        lines = content.split('\n')
        new_lines = []
        for line in lines:
            new_lines.append(self.processLine(line))

        return '\n'.join(new_lines)

    def play(self):
        return self.process(self.content)
