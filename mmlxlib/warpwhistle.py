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
import re, os, math
from util import Util
from instrument import Instrument

class WarpWhistle(object):
    TEMPO = 'tempo'
    VOLUME = 'volume'
    TIMBRE = 'timbre'
    ARPEGGIO = 'arpeggio'
    INSTRUMENT = 'instrument'
    PITCH = 'pitch'
    OCTAVE = 'octave'
    SLIDE = 'slide'
    Q = 'q'

    ABSOLUTE_NOTES = 'X-ABSOLUTE-NOTES'
    TRANSPOSE = 'X-TRANSPOSE'
    COUNTER = 'X-COUNTER'
    X_TEMPO = 'X-TEMPO'
    SMOOTH = 'X-SMOOTH'
    N106 = 'EX-NAMCO106'
    FDS = 'EX-DISKFM'
    VRC6 = 'EX-VRC6'
    PITCH_CORRECTION = 'PITCH-CORRECTION'

    CHIP_N106 = 'N106'
    CHIP_FDS = 'FDS'
    CHIP_VRC6 = 'VRC6'

    def __init__(self, content, logger, options):
        self.first_run = True

        # current voice we are processing if we are processing voices separately
        self.process_voice = None

        # list of voices to process
        self.voices_to_process = None

        # list of voices
        self.voices = None

        self.content = content
        self.logger = logger
        self.options = options
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
            filename = match[2] if match[2].endswith('.mmlx') else match[2] + '.mmlx'
            disk_path = os.path.join(self.import_directory, filename)
            file_content = Util.openFile(disk_path)
            content = content.replace(match[0], file_content)

        self.logger.log('- stripping comments again', True)
        content = self.stripComments(content)

        if content.find('@import') > 0:
            content = self.processImports(content)

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
        matches = re.findall(r'(^#([-A-Z0-9]+)( {1,}(.*))?\n)', content, re.MULTILINE)
        for match in matches:
            if match[1] == WarpWhistle.TRANSPOSE:
                self.global_vars[match[1]] = int(match[3]) if match[3] else 0
            else:
                self.global_vars[match[1]] = match[3] or True

            if match[1].startswith('X-'):
                content = content.replace(match[0], '')
            else:
                self.global_lines.append(match[0])

            if match[1] == WarpWhistle.COUNTER:
                Instrument.reset(match[3])

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
        if name == 'end':
            raise Exception('end is a reserved word and connt be used for an instrument')

        lines = content.strip().split('\n')
        data = {}

        for line in lines:
            line = line.strip()
            match = re.match(r'^@extends {1,}(\'|\")(.*)(\1)$', line)
            if match:
                data["extends"] = match.group(2)
                continue

            data[line.split(':', 1)[0].strip()] = line.split(':', 1)[1].strip()

        self.instruments[name] = Instrument(data)

    def updateInstruments(self):
        for name in self.instruments:
            instrument = self.instruments[name]
            while instrument.hasParent():
                instrument.inherit(self.instruments[instrument.getParent()])

    def processInstruments(self, content):
        matches = re.findall(r'(^([a-zA-Z0-9-_]+):( {0,}(\n( {4}|\t)(.*))+)\n)', content, re.MULTILINE)
        for match in matches:
            self.addInstrument(match[1].lower(), match[2])
            content = content.replace(match[0], '')

        self.updateInstruments()
        return content

    def getVoicesForChip(self, chip):
        if chip == WarpWhistle.CHIP_N106:
            return {
                'A': 'P',
                'B': 'Q',
                'C': 'R',
                'D': 'S',
                'E': 'T',
                'F': 'U',
                'G': 'V',
                'H': 'W'
            }

        if chip == WarpWhistle.CHIP_FDS:
            return {
                'A': 'F'
            }

        if chip == WarpWhistle.CHIP_VRC6:
            return {
                'A': 'M',
                'B': 'N',
                'C': 'O'
            }

    def getVoiceTranslation(self, chip, voice):
        voices = self.getVoicesForChip(chip)

        if not voice in voices:
            return None

        return voices[voice]

    def getVoiceFor(self, chip, voices):
        final_voices = []
        voices = list(voices)
        for voice in voices:
            new_voice = self.getVoiceTranslation(chip, voice)
            if new_voice is None:
                raise Exception('voice: ' + voice + ' is outside of the voice range for chip: ' + chip)

            final_voices.append(new_voice)

        return ''.join(final_voices)

    def processExpansionVoices(self, content):
        """finds any special voices (such as N106-AB) and converts them to the proper voice names"""
        matches = re.findall(r'((N106|FDS|VRC6)-([A-Z]+) )', content)
        for match in matches:
            content = content.replace(match[0], self.getVoiceFor(match[1], match[2]) + ' ')

        return content

    def renderTempo(self, content):
        tempo = self.getGlobalVar(WarpWhistle.X_TEMPO)
        if tempo is None:
            return content

        return self.addToMml(content, "".join(self.voices) + " t" + str(tempo) + "\n")

    def renderInstruments(self, content):
        if not Instrument.hasBeenUsed():
            return content

        # find the last #BLOCK on the top of the file and render the instruments below it
        return self.addToMml(content, Instrument.render())

    def renderN106(self, content):
        n106_voices = self.getVoicesForChip(WarpWhistle.CHIP_N106).values()
        n106_voices.sort()

        n106_count = 0
        for voice in self.voices:
            if voice in n106_voices:
                index = n106_voices.index(voice) + 1
                n106_count = max(n106_count, index)

        if n106_count > 0:
            # in order to stay on pitch it has to be 1,2,4 or 8
            if n106_count == 3:
                n106_count = 4

            if n106_count in [5, 6, 7]:
                n106_count = 8

            if not WarpWhistle.N106 in self.global_vars:
                self.global_vars[WarpWhistle.N106] = str(n106_count)
                content = self.addToMml(content, '#' + WarpWhistle.N106 + ' ' + str(n106_count) + '\n', True)

            if not WarpWhistle.PITCH_CORRECTION in self.global_vars:
                self.global_vars[WarpWhistle.PITCH_CORRECTION] = True
                content = self.addToMml(content, '#' + WarpWhistle.PITCH_CORRECTION + '\n', True)

        return content

    def getExpForChip(self, chip):
        if chip == WarpWhistle.CHIP_N106:
            return WarpWhistle.N106
        elif chip == WarpWhistle.CHIP_FDS:
            return WarpWhistle.FDS
        elif chip == WarpWhistle.CHIP_VRC6:
            return WarpWhistle.VRC6

    def renderForChip(self, chip, content):
        chip_voices = self.getVoicesForChip(chip).values()
        chip_voices.sort()

        used = False
        for voice in self.voices:
            if voice in chip_voices:
                used = True
                break

        exp = self.getExpForChip(chip)
        if used and not exp in self.global_vars:
            self.global_vars[exp] = True
            content = self.addToMml(content, '#' + exp + '\n', True)

        return content

    def renderExpansionChips(self, content):
        content = self.renderN106(content)
        content = self.renderForChip(WarpWhistle.CHIP_FDS, content)
        content = self.renderForChip(WarpWhistle.CHIP_VRC6, content)

        return content

    def addToMml(self, content, string, is_global_line = False):
        try:
            last_global_declaration = self.global_lines[-1]
        except:
            # if there are no global lines then prepend
            return string + content

        if is_global_line:
            self.global_lines.append(string)

        return content.replace(last_global_declaration, last_global_declaration + string)

    def replaceVariables(self, content):
        for key in self.vars:
            pattern = '((?<=\s)|(?<=\[))' + key + '(?=\s|\Z|\])'
            content = re.sub(re.compile(pattern, re.MULTILINE), self.vars[key], content)

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

    def getFrequency(self, note, octave):
        frequencies = [
            0x06AE,
            0x064E,
            0x05F4,
            0x059E,
            0x054E,
            0x0501,
            0x04B9,
            0x0476,
            0x0436,
            0x03F9,
            0x03C0,
            0x038A
        ]

        index = self.getNumberForNote()[note]
        frequency = frequencies[index] >> (octave - 2)
        return frequency

    def calculateN106OctaveShift(self, channel_count, waveform):
        if waveform is None:
            return 0

        # my math skills are so lacking these days this is the number of octaves to shift
        # based on the waveform sample count and the channel count
        shift = {
            4: 4,
            8: 3,
            16: 2,
            32: 1,
            64: 0,
            128: -1,
            256: -2
        }

        number = channel_count * len(waveform.strip().split(' '))
        return shift[number]

    def slide(self, start_data, end_data):
        N106_channels = self.getGlobalVar(WarpWhistle.N106)
        shift = 0
        if N106_channels is not None and len(N106_channels):
            active_instruments = self.getDataForVoice(self.current_voices[0], WarpWhistle.INSTRUMENT)
            waveform = None
            for instrument in active_instruments:
                if hasattr(instrument, 'waveform'):
                    waveform = instrument.waveform
                    break

            shift = self.calculateN106OctaveShift(int(N106_channels), waveform)

        match = re.match(r'^(\[+)?([a-g](\+|\-)?)(.*)$', start_data['note'])
        start_data['note'] = match.group(2)
        start_data['append'] = match.group(4)

        # total amount we need to move
        slide_amount = self.getFrequency(start_data['note'], start_data['octave'] + shift) - self.getFrequency(end_data['note'], end_data['octave'] + shift)

        # song tempo
        tempo = self.getDataForVoice(self.current_voices[0], WarpWhistle.TEMPO)

        # make sure if tempo is none we default to 120
        if tempo is None:
            tempo = 120

        # @todo this is NTSC, need to add support for PAL - 50 FPS
        frames_per_second = 60

        # default to 16th note unless speed is specified
        slide_note_duration = 16 if start_data['speed'] is None else start_data['speed']

        # figure out the slide duration in seconds
        beats_per_second = float(tempo) / float(60)
        slides_per_second = (float(slide_note_duration) / float(4)) * beats_per_second
        slide_duration = float(1) / float(slides_per_second)

        frames_for_slide = int(math.floor(frames_per_second * slide_duration))

        steps = frames_for_slide
        distance_per_step = int(math.floor(slide_amount / frames_for_slide))
        remainder = slide_amount - steps * distance_per_step

        # print 'STEPS',steps
        # print 'DISTANCE_PER_STEP',distance_per_step
        # print 'REMAINDER',remainder

        increase_at = steps - remainder
        pitch_macro = []
        for x in range(0, steps):
            if x < increase_at:
                pitch_macro.append(str(distance_per_step))
                continue

            pitch_macro.append(str(distance_per_step + 1))

        pitch_macro = ' '.join(pitch_macro) + ' 0'
        instrument = Instrument({
            'pitch': pitch_macro
        })

        macro = instrument.getPitchMacro()

        # no longer need to slide
        self.setDataForVoices(self.current_voices, WarpWhistle.SLIDE, None)

        append_before = end_data['append']
        append_after =''
        match = re.match(r'(.*)(\](.*))', end_data['append'])
        if match:

            if match.group(1):
                append_before = match.group(1)

            if match.group(2):
                append_after = match.group(2)

        # print 'START',start_data
        # print 'END',end_data
        # print ""

        octave_diff = start_data['octave'] - end_data['octave']

        diff = Util.arrayDiff(self.current_voices, ['A', 'B', 'C'])

        smooth_start = ''
        smooth_end = ''
        if len(diff) == 0:
            smooth = self.getGlobalVar(WarpWhistle.SMOOTH)

            if smooth:
                smooth_start = ' SM '
                smooth_end = ' SMOF'

        return self.getOctaveShift(octave_diff) + smooth_start + macro + ' ' + start_data['note'] + append_before + ' EPOF' + smooth_end + append_after + ' ' + self.getOctaveShift(-octave_diff)

    def getOctaveShift(self, ticks):
        char = '<' if ticks < 0 else '>'
        return abs(ticks) * char

    def transposeNote(self, note, octave, amount, append):
        if append is None:
            append = ''

        start_data = self.getDataForVoice(self.current_voices[0], WarpWhistle.SLIDE)

        new_note = ''
        if amount == 0 or self.isNoiseChannel():
            if start_data:
                return self.slide(start_data, {'note': note, 'append': append, 'octave': octave})

            return note + append

        new_note_number = self.getNumberForNote()[note] + amount

        ticks = 0
        while new_note_number < 0:
            new_note += '< '
            ticks += 1
            new_note_number = new_note_number + 12

        while new_note_number > 11:
            ticks  -= 1
            new_note += '> '
            new_note_number = new_note_number - 12

        note_name = self.getNoteForNumber()[new_note_number]
        new_note += note_name
        new_note += append + ' ' + self.getOctaveShift(ticks)

        return new_note

    def processWord(self, word, next_word, prev_word):
        if not word:
            return word

        valid_commands = ['EPOF', 'ENOF', 'MPOF', 'PS', 'SDQR', 'SDOF', 'MHOF', 'SM', 'SMOF', 'EHOF']
        if word in valid_commands:
            if self.ignore:
                return ""

            return word

        # matches a voice declaration
        if re.match(r'[A-Z]{1,}$', word):

            self.current_voices = list(word)

            # processing everything, keep going
            if self.process_voice is None:
                return word

            # if we are processing a specific voice
            # and we are on that voice
            if self.process_voice in self.current_voices:
                self.ignore = False
                return self.process_voice

            # if we are processing a specific voice and we are not on that voice
            self.ignore = True
            return ""

        if self.ignore:
            return ""

        # slides for portamento
        match = re.match(r'^\/([0-9]+)?$', word)
        if match:

            # calculate the previous note
            prev_note = self.processWord(prev_word, None, None)

            # figure out what octave we are at now
            start_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)

            self.setDataForVoices(self.current_voices, WarpWhistle.SLIDE, {'note': prev_note, 'octave': start_octave, 'speed': match.group(1)})

            return ''

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

        # arpeggio change
        if re.match(r'EN\d+$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.ARPEGGIO, int(word[2:]))
            return word

        # pitch change
        if re.match(r'EP\d+$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.PITCH, int(word[2:]))
            return word

        # q change
        if re.match(r'q[0-8]$', word):
            self.setDataForVoices(self.current_voices, WarpWhistle.Q, int(word[1:]))
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
        if re.match(r'\>+|\<+$', word):
            direction = word[0]
            count = len(word)
            current_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)

            if current_octave is None:
                current_octave = 0

            self.setDataForVoices(self.current_voices, WarpWhistle.OCTAVE, current_octave + (count if direction == '>' else -count))

            return word

        # dmc declaration
        match = re.match(r'(\{\s{0,})?(\'|\")(.*\.dmc)(\2)\s{0,},', word)
        if match:
            mmlx_dir = self.options['start'] if os.path.isdir(self.options['start']) else os.path.dirname(self.options['start'])
            new_path = os.path.join(mmlx_dir, match.group(3))
            new_word = ''

            if match.group(1):
                new_word += match.group(1)

            new_word += match.group(2) + new_path + match.group(2) + ','

            return new_word

        # rewrite special voices for mmlx such as c4 or G+,4^8
        # to use this put the line X-ABSOLUTE-NOTES at the top of your mmlx file
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
        match = re.match(r'(\[+)?([a-g]{1}(\+|\-)?)([\.0-9\^]+)?([\]\d+]+)?$', word)
        if match:
            if "," in word and not self.getGlobalVar(WarpWhistle.ABSOLUTE_NOTES):
                raise Exception('In order to use absolute notes you have to specify X-ABSOLUTE-NOTES')

            current_octave = self.getDataForVoice(self.current_voices[0], WarpWhistle.OCTAVE)

            new_note = ""
            if match.group(1):
                new_note += match.group(1)

            append = ''
            if match.group(4):
                append = match.group(4)

            if match.group(5):
                append += match.group(5)

            new_note += self.transposeNote(match.group(2), current_octave, self.getGlobalVar(WarpWhistle.TRANSPOSE), append)

            return new_note

        # instrument
        match = re.match(r'^(\[+)?(\+)?@([a-zA-Z0-9-_]+)([\]\d+]+)?$', word)
        if match:

            new_word = ''

            # special case if you do @end you can end the currently active instruments
            if match.group(3) == 'end':
                active_instruments = self.getDataForVoice(self.current_voices[0], WarpWhistle.INSTRUMENT)

                for active_instrument in active_instruments:
                    new_word += active_instrument.end(self)

                self.setDataForVoices(self.current_voices, WarpWhistle.INSTRUMENT, [])
                return new_word

            # not a valid instrument
            if not match.group(3) in self.instruments:
                return word

            new_instrument = self.instruments[match.group(3)]

            if 'O' in self.current_voices and hasattr(new_instrument, 'timbre'):
                raise Exception('VRC6 sawtooth (voice O) does not support timbre attribute')

            chip = new_instrument.getChip()
            diff = []
            if chip is not None:
                diff = Util.arrayDiff(self.current_voices, self.getVoicesForChip(chip).values())

            if len(diff):
                diff.sort()
                words = ('voice', 'does') if len(diff) == 1 else ('voices', 'do')
                raise Exception(words[0] + ' ' + ', '.join(diff) + ' ' + words[1] + ' not support instruments using chip: ' + chip)

            active_instruments = self.getDataForVoice(self.current_voices[0], WarpWhistle.INSTRUMENT)

            if active_instruments is None:
                active_instruments = []

            if match.group(1):
                new_word += match.group(1)

            if len(active_instruments) and not match.group(2):
                for active_instrument in active_instruments:
                    new_word += active_instrument.end(self)

                active_instruments = []

            active_instruments.append(new_instrument)
            self.setDataForVoices(self.current_voices, WarpWhistle.INSTRUMENT, active_instruments)

            new_word += new_instrument.start(self)

            if match.group(4):
                new_word += match.group(4)

            return new_word

        if self.isUndefinedVariable(word):
            raise Exception('variable ' + word + ' is undefined')

        # print "PROCESS:",word
        # print "PREV:",prev_word
        # print "NEXT:",next_word
        # print ""
        return word

    def processLine(self, line):
        self.ignore = False

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

    def findVoices(self, content):
        matches = re.findall(r'^([A-Z]{1,}) ', content, re.MULTILINE)

        voices = []
        for match in matches:
            new_voices = list(match)
            for voice in new_voices:
                if not voice in voices:
                    voices.append(voice)

        return voices

    def process(self, content):
        self.logger.log('- stripping comments', True)
        content = self.stripComments(content)

        self.logger.log('- proccessing imports', True)
        content = self.processImports(content)

        self.logger.log('- parsing variables', True)
        content = self.processVariables(content)

        self.logger.log('- applying variables', True)
        content = self.replaceVariables(content)

        self.logger.log('- parsing instruments', True)
        content = self.processInstruments(content)

        self.logger.log('- collapsing spaces', True)
        content = self.collapseSpaces(content)

        self.logger.log('- processing expansion voices', True)
        content = self.processExpansionVoices(content)

        self.voices = self.findVoices(content)
        content = self.renderTempo(content)

        content = self.renderExpansionChips(content)

        if not self.first_run:
            if self.voices_to_process is None:
                self.voices_to_process = self.voices

            if len(self.voices_to_process):
                self.process_voice = self.voices_to_process.pop(0)

        if self.process_voice:
            self.logger.log('processing voice: ' + self.process_voice, True)

        lines = content.split('\n')
        new_lines = []
        for line in lines:
            new_lines.append(self.processLine(line))

        content = '\n'.join(new_lines)

        content = self.renderInstruments(content)

        self.logger.log('- replacing unneccessary octave shifts', True)
        patterns = ['><', '> <', '<>', '< >']
        for pattern in patterns:
            while content.find(pattern) > 0:
                content = content.replace(pattern, '')

        self.logger.log('- replacing extra spaces', True)
        content = self.collapseSpaces(content)

        self.logger.log('- removing blank lines', True)
        content = self.removeBlankLines(content)

        self.first_run = False

        return content

    def isPlaying(self):
        if self.first_run:
            return True

        if not self.options['separate_voices']:
            return False

        return self.voices_to_process is None or len(self.voices_to_process) != 0

    def play(self):
        counter = self.getGlobalVar(WarpWhistle.COUNTER) or 0
        Instrument.reset(counter)
        self.reset()
        return (self.process(self.content), self.process_voice)
