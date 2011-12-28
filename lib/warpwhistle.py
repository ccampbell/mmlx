import re, os, math
from util import Util
from instrument import Instrument

class WarpWhistle(object):
    TEMPO = 'tempo'
    VOLUME = 'volume'
    TIMBRE = 'timbre'
    INSTRUMENT = 'instrument'
    OCTAVE = 'octave'
    ABSOLUTE_NOTES = 'X-ABSOLUTE-NOTES'
    TRANSPOSE = 'X-TRANSPOSE'

    def __init__(self, content, logger):
        self.content = content
        self.logger = logger
        self.reset()

    def reset(self):
        self.current_voices = []
        self.global_vars = {}
        self.vars = {}
        self.instruments = {}
        self.data = {}
        self.global_lines = []

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

    def getVar(self, key):
        if key in self.vars:
            return self.vars[key]

        return None

    def getGlobalVar(self, key):
        if key in self.global_vars:
            return self.global_vars[key]

        if key == WarpWhistle.TRANSPOSE:
            return 0

        return None

    def processImports(self, content):
        matches = re.findall(r'(@import\s{1,}(\'|\")(.*)(\2))$', content, re.MULTILINE)

        for match in matches:
            filename = match[2] if match[2].endswith('.xmml') else match[2] + '.xmml'
            disk_path = os.path.join(self.import_directory, filename)
            file_content = Util.openFile(disk_path)
            content = content.replace(match[0], file_content)

        return content

    def stripComments(self, content):
        # replace all /* comments */
        content = re.sub(re.compile(r'(/\*(.*?)\*/)', re.MULTILINE | re.DOTALL), '', content)

        # replace all ; comments
        content = re.sub(re.compile(r' {0,}(;.*)$', re.MULTILINE), '', content)

        # replace all // comments
        content = re.sub(re.compile(r' {0,}(//.*)$', re.MULTILINE), '', content)

        # replace empty lines
        content = re.sub(re.compile(r'\n{2,}', re.MULTILINE), '\n', content)

        return content

    def collapseSpaces(self, content):
        # collapse multiple spaces into a single space
        return re.sub(re.compile(' {2,}', re.MULTILINE), ' ', content)

    def removeBlankLines(self, content):
        return re.sub(r'\n{2,}', '\n', content)

    def processGlobalVariables(self, content):
        matches = re.findall(r'(^#([-A-Z]+)( {1,}(.*))?\n)', content, re.MULTILINE)
        for match in matches:
            if match[1] == WarpWhistle.TRANSPOSE:
                self.global_vars[match[1]] = int(match[3]) if match[3] else 0
            else:
                self.global_vars[match[1]] = match[3] or True

            if match[1].startswith('X-'):
                content = content.replace(match[0], '')
            else:
                self.global_lines.append(match[0])

        return content

    def isReserved(self, var):
        # single letter match
        if re.match(r'^[A-Z]{1,2}$', var):
            return True

        # volume macro
        if re.match(r'^v\d+$', var):
            return True

        # rest
        if re.match(r'^r\d+$', var):
            return True

        # wait
        if re.match(r'^w\d+$', var):
            return True

        # pitch macro
        if re.match(r'^EP\d+$', var):
            return True

        # arpeggio macro
        if re.match(r'^EN\d+$', var):
            return True

        # self delay macro
        if re.match(r'^SD\d+$', var):
            return True

        reserved = ['EPOF', 'ENOF', 'SDOF', 'w', 'r']

        return var in reserved

    def processLocalVariables(self, content):
        matches = re.findall(r'(^([a-zA-Z]{1}([a-zA-Z0-9_]+)?)\s{0,}=\s{0,}(.*)\n)', content, re.MULTILINE)
        for match in matches:

            if self.isReserved(match[1]):
                raise Exception('variable ' + match[1] + ' is reserved')

            self.vars[match[1]] = match[3]

            content = content.replace(match[0], '')

        return content

    def processVariables(self, content):
        content = self.processGlobalVariables(content)
        content = self.processLocalVariables(content)
        return content

    def addInstrument(self, name, content):
        lines = content.strip().split('\n')
        data = {}

        for line in lines:
            line = line.strip()
            match = re.match(r'^@extends {1,}(\'|\")(.*)(\1)$', line)
            if match:
                data["extends"] = match.group(2)
                continue

            data[line.split(':')[0].strip()] = line.split(':')[1].strip()

        self.instruments[name] = Instrument(data)

    def updateInstruments(self):
        for name in self.instruments:
            instrument = self.instruments[name]
            while instrument.hasParent():
                instrument.inherit(self.instruments[instrument.getParent()])

    def processInstruments(self, content):
        matches = re.findall(r'(^([a-zA-Z0-9-_]+):( {0,}(\n( {4}|\t)(.*))+)\n)', content, re.MULTILINE)
        for match in matches:
            self.addInstrument(match[1], match[2])
            content = content.replace(match[0], '')

        self.updateInstruments()
        return content

    def renderInstruments(self, content):
        if not Instrument.hasBeenUsed():
            return content

        # find the last #BLOCK on the top of the file and render the instruments below it
        last_global_declaration = self.global_lines[-1]
        return content.replace(last_global_declaration, last_global_declaration + Instrument.render())

    def replaceVariables(self, content):
        for key in self.vars:
            content = content.replace(key, self.vars[key])

        return content

    def isUndefinedVariable(self, var):
        return False

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

    def getNumberForNote(self):
        return {
            'c': 0,
            'c+': 1,
            'd-': 1,
            'd': 2,
            'd+': 3,
            'e-': 3,
            'e': 4,
            'f-': 4,
            'e+': 5,
            'f': 5,
            'f+': 6,
            'g-': 6,
            'g': 7,
            'g+': 8,
            'a-': 8,
            'a': 9,
            'a+': 10,
            'b-': 10,
            'b': 11
        }

    def getNoteForNumber(self):
        return {
            0: 'c',
            1: 'c+',
            2: 'd',
            3: 'd+',
            4: 'e',
            5: 'f',
            6: 'f+',
            7: 'g',
            8: 'g+',
            9: 'a',
            10: 'a+',
            11: 'b'
        }

    def isNoiseChannel(self):
        return self.current_voices[0] == 'D'

    def transposeNote(self, note, octave, amount, append):
        if append is None:
            append = ''

        if amount == 0 or self.isNoiseChannel():
            return note + append

        new_note_number = self.getNumberForNote()[note] + amount
        new_note = ""


        ticks = 0
        while new_note_number < 0:
            new_note += '< '
            ticks += 1
            new_note_number = new_note_number + 12

        while new_note_number > 11:
            ticks  -= 1
            new_note += '> '
            new_note_number = new_note_number - 12

        new_note += self.getNoteForNumber()[new_note_number]

        char = '<' if ticks < 0 else '>'
        new_note += append + ' ' + abs(ticks) * char

        return new_note

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

        # direct timbre
        if re.match(r'@\d+$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.TIMBRE, int(word[1:]))
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

        # rewrite special voices for xmml such as c4 or G+/4^8
        # to use this put the line X-ABSOLUTE-NOTES at the top of your xmml file
        match = re.match(r'(\[+)?([A-Ga-g]{1})(\+|\-)?(\d{1,2})?(,(\d+\.?)(\^[0-9\^]+)?)?([\]\d]+)?$', word)
        if match and self.getGlobalVar(WarpWhistle.ABSOLUTE_NOTES):
            is_noise_channel = self.current_voices[0] == 'D'

            if is_noise_channel and not "," in word:
                return word

            new_word = ""

            octave = match.group(4) if not is_noise_channel else 0

            current_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)

            if current_octave is None and not is_noise_channel:
                new_word += 'o' + octave + ' '
            elif not is_noise_channel and octave and int(octave) != current_octave:
                new_word += self.moveToOctave(int(octave), current_octave) + ' '

            if octave:
                self.setDataForVoices(self.current_voices, WarpWhistle.OCTAVE, int(octave))
                current_octave = int(octave)

            # [[[
            if match.group(1):
                new_word += match.group(1)

            note = ""

            # note
            note += match.group(2).lower()

            # accidental
            if match.group(3):
                note += match.group(3)

            append = ""

            # tack on the note length
            if match.group(6):
                append += match.group(6)

            # tack on any ties (such as ^8^16)
            if match.group(7):
                append +=  match.group(7)

            # tack on the final repeat value if it is present (]4)
            if match.group(8):
                append += match.group(8)

            new_word += self.transposeNote(note, current_octave, self.getGlobalVar(WarpWhistle.TRANSPOSE), append)

            return new_word

        # regular note
        match = re.match(r'(\[+)?([a-g]{1}(\+|\-)?)([\.0-9\^]+)?([\]\d]+)?', word)
        if match:
            if "," in word and not self.getGlobalVar(WarpWhistle.ABSOLUTE_NOTES):
                raise Exception('In order to use absolute notes you have to specify X-ABSOLUTE-NOTES')

            current_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)

            new_note = ""
            if match.group(1):
                new_note += match.group(1)

            new_note += self.transposeNote(match.group(2), current_octave, self.getGlobalVar(WarpWhistle.TRANSPOSE), match.group(4))

            return new_note

        match = re.match(r'^(\[+)?@([a-zA-Z0-9-_]+)$', word)
        if match:
            new_instrument = self.instruments[match.group(2)]
            active_instrument = self.getDataForVoice(self.current_voices[0], WarpWhistle.INSTRUMENT)
            self.setDataForVoices(self.current_voices, WarpWhistle.INSTRUMENT, new_instrument)

            new_word = ''

            if match.group(1):
                new_word += match.group(1)

            if active_instrument:
                new_word += active_instrument.end()

            new_word += new_instrument.start()

            return new_word

        if self.isUndefinedVariable(word):
            raise Exception('variable ' + word + ' is undefined')

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

        self.logger.log('proccessing imports', True)
        content = self.processImports(content)

        self.logger.log('stripping comments', True)
        content = self.stripComments(content)

        self.logger.log('parsing instruments', True)
        content = self.processInstruments(content)

        self.logger.log('parsing variables', True)
        content = self.processVariables(content)

        self.logger.log('applying variables', True)
        content = self.replaceVariables(content)

        self.logger.log('collapsing spaces', True)
        content = self.collapseSpaces(content)

        lines = content.split('\n')
        new_lines = []
        for line in lines:
            new_lines.append(self.processLine(line))

        content = '\n'.join(new_lines)

        content = self.renderInstruments(content)

        self.logger.log('replace unneccessary octave shifts', True)
        levels = math.ceil(abs(self.getGlobalVar(WarpWhistle.TRANSPOSE)) / 12)
        for x in range(0, int(levels + 1)):
            content = self.collapseSpaces(content)
            content = content.replace('> <', '').replace('< >', '')

        content = self.collapseSpaces(content)

        self.logger.log('removing blank lines', True)
        content = self.removeBlankLines(content)

        return content

    def play(self):
        return self.process(self.content)
