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
from util import Util
import math
import re


class Instrument(object):

    def __init__(self, data):
        valid_chips = ['N106', 'FDS', 'VRC6']

        for key in data:
            if key == 'chip' and data[key] not in valid_chips:
                raise Exception('value for chip is not valid: ' + data[key])

            setattr(self, key, data[key])

        if hasattr(self, 'adsr'):
            self.volume = self.getVolumeFromADSR(self.adsr)

        if hasattr(self, 'volume'):
            self.volume = self.magicMacro(self.volume)

        if self.getChip() == 'N106':
            Instrument.N106_buffers[self.waveform] = int(self.buffer) if hasattr(self, 'buffer') else None

    # attack - time taken for amplitude to rise from 0 to max (15)
    # decay - time taken for amplitude to drop to sustain level
    # sustain - amplitude at which the note is held
    # release - time taken for amplitude to drop from sustain level to 0
    #
    # note:
    # if decay is 0, max amplitude is the sustain value
    #
    def getVolumeFromADSR(self, adsr):
        bits = adsr.split(' ')
        attack = bits[0]
        decay = bits[1]
        sustain = bits[2]
        release = bits[3]

        max_volume = sustain if int(decay) == 0 else 15

        if hasattr(self, "max_volume"):
            max_volume = int(self.max_volume)

        if attack == '0' and decay == '0' and sustain == '0' and release != '0':
            sustain = 15

        volume = ''
        if attack != '0':
            volume += str(max_volume) + ' ' if attack == '0' else ' '.join(self.divideIntoSteps(0, max_volume, attack)) + ' '

        if sustain != '0':
            volume += ' '.join(self.divideIntoSteps(max_volume, sustain, decay)) + ' '

        if release != '0':
            volume += ' '.join(self.divideIntoSteps(sustain, 0, release)) + ' '

        if volume.strip() == '':
            volume = '0'

        return volume

    def divideIntoSteps(self, min, max, steps):
        min = int(min)
        max = int(max)
        steps = int(steps)

        # print 'MIN', min
        # print 'MAX', max
        # print 'STEPS', steps

        if steps == 1:
            return [str(min), str(max)]

        # figure out the equation of a line to match these coordinates
        # coordinates are (0, min) and (steps - 1, max)
        slope = float(max - min) / float(steps - 1)
        equation = lambda x: slope * x + min

        values = []
        for x in range(0, steps):
            value = int(math.ceil(equation(x)))
            values.append(str(value))

        # print values
        return values

    # def processRepeats(self, macro):
    #     if not "'" in macro:
    #         return macro

    #     # group repeats
    #     match = re.match(r'\((.*)\)\'(\d+)', macro)

    #     if not match:

    #         # single repeat
    #         match = re.match(r'\b(.*?)\'(\d+)', macro)

    #     if not match:
    #         return macro
    #     print 'GROUP1',match.group(1)
    #     print 'GROUP2',match.group(2)
    #     repeats = int(match.group(2))
    #     pattern = match.group(1)
    #     replacement = (pattern + ' ') * repeats

    #     return self.processRepeats(macro.replace(match.group(0), replacement).replace('  ', ' '))

    def magicMacroObjects(self, macro):
        # bracket objects
        match = re.match(r'(\[(.*)\](\.[a-zA-Z]{1}.*\))+)', macro)

        if not match:
            # no bracket
            match = re.match(r'((.*?)(\.[a-zA-Z]{1}.*\))+)', macro)

        if match:
            # print 'MACRO',macro
            # print 'GROUP1',match.group(1)
            # print 'GROUP2',match.group(2)
            # print 'GROUP3',match.group(3)

            # print 'GROUP4',match.group(4)
            magic = MagicMacro(macro.replace(match.group(1), self.magicMacroObjects(match.group(2))))

            methods = match.group(3)[1:-1].split(').')
            for method in methods:
                getattr(magic, method.split('(')[0])(*method.split('(')[1].split(','))

            return str(magic)

        # print macro
        return str(MagicMacro(macro))

    def magicMacro(self, macro):
        # macro = self.processRepeats(macro)
        return self.magicMacroObjects(macro)
        # print 'RESULT',test


    def getChip(self):
        if hasattr(self, 'chip'):
            return self.chip

        return None

    @staticmethod
    def reset(counter=0):
        counter = int(counter)

        Instrument.counters = {
            'timbre': counter,
            'volume': counter,
            'pitch': counter,
            'arpeggio': counter,
            'vibrato': counter,
            'N106': counter,
            'FDS': counter
        }

        Instrument.timbres = {}
        Instrument.volumes = {}
        Instrument.pitches = {}
        Instrument.arpeggios = {}
        Instrument.vibratos = {}
        Instrument.N106 = {}
        Instrument.N106_buffers = {}
        Instrument.FDS = {}

    def hasParent(self):
        return hasattr(self, 'extends') and self.extends is not None

    def inherit(self, instrument):
        dictionary = instrument.__dict__
        for key in dictionary:
            if key == "extends" or not hasattr(self, key):
                setattr(self, key, dictionary[key])

        if not "extends" in dictionary:
            delattr(self, "extends")

    def getParent(self):
        return self.extends

    def getCountFor(self, macro):
        i = Instrument.counters[macro]
        Instrument.counters[macro] += 1
        return i

    def getVolumeMacro(self):
        i = Instrument.volumes[self.volume] if self.volume in Instrument.volumes else self.getCountFor('volume')
        Instrument.volumes[self.volume] = i
        return '@v' + str(i)

    def getPitchMacro(self):
        i = Instrument.pitches[self.pitch] if self.pitch in Instrument.pitches else self.getCountFor('pitch')
        Instrument.pitches[self.pitch] = i
        return 'EP' + str(i)

    def getArpeggioMacro(self):
        i = Instrument.arpeggios[self.arpeggio] if self.arpeggio in Instrument.arpeggios else self.getCountFor('arpeggio')
        Instrument.arpeggios[self.arpeggio] = i
        return 'EN' + str(i)

    def getTimbreMacro(self):
        i = Instrument.timbres[self.timbre] if self.timbre in Instrument.timbres else self.getCountFor('timbre')
        Instrument.timbres[self.timbre] = i
        return '@@' + str(i)

    def getVibratoMacro(self):
        i = Instrument.vibratos[self.vibrato] if self.vibrato in Instrument.vibratos else self.getCountFor('vibrato')
        Instrument.vibratos[self.vibrato] = i
        return 'MP' + str(i)

    def getN106Macro(self):
        i = Instrument.N106[self.waveform] if self.waveform in Instrument.N106 else self.getCountFor('N106')
        Instrument.N106[self.waveform] = i
        return '@@' + str(i)

    def getFDSMacro(self):
        i = Instrument.FDS[self.waveform] if self.waveform in Instrument.FDS else self.getCountFor('FDS')
        Instrument.FDS[self.waveform] = i
        return '@@' + str(i)

    @staticmethod
    def hasBeenUsed():
        macros = ["timbres", "volumes", "pitches", "arpeggios", "vibratos", "N106", "FDS"]
        for macro in macros:
            if len(getattr(Instrument, macro)) > 0:
                return True

        return False

    @staticmethod
    def render():
        macros = ''

        # render timbres
        for timbre in Util.sortDictionary(Instrument.timbres):
            macros += '@' + str(timbre[1]) + ' = { ' + timbre[0] + ' }\n'

        # render volumes
        for volume in Util.sortDictionary(Instrument.volumes):
            macros += '@v' + str(volume[1]) + ' = { ' + volume[0] + ' }\n'

        # render pitches
        for pitch in Util.sortDictionary(Instrument.pitches):
            macros += '@EP' + str(pitch[1]) + ' = { ' + pitch[0] + ' }\n'

        # render arpeggios
        for arpeggio in Util.sortDictionary(Instrument.arpeggios):
            macros += '@EN' + str(arpeggio[1]) + ' = { ' + arpeggio[0] + ' }\n'

        # render vibratos
        for vibrato in Util.sortDictionary(Instrument.vibratos):
            macros += '@MP' + str(vibrato[1]) + ' = { ' + vibrato[0] + ' }\n'

        # render N106
        for macro in Util.sortDictionary(Instrument.N106):
            waveform = Instrument.validateN106(macro[0])
            macros += '@N' + str(macro[1]) + ' = { ' + Instrument.getN106Buffer(waveform) + ', ' + waveform + ' }\n'

        # render FDS
        for macro in Util.sortDictionary(Instrument.FDS):
            macros += '@FM' + str(macro[1]) + ' = { ' + Instrument.validateFds(macro[0]) + ' }\n'

        return macros

    @staticmethod
    def validateN106(macro):
        bits = macro.strip().split(' ')
        if len(bits) % 4 != 0:
            raise Exception('N106 waveform samples have to be a multiple of 4')

        for bit in bits:
            bit = int(bit.replace('$', ''), 16) if bit.startswith('$') else int(bit)
            if bit < 0:
                raise Exception('N106 waveform parameter cannot be less than 0')

            if bit > 15:
                raise Exception('N106 waveform parameter cannot be greater than 15')

        return macro

    @staticmethod
    def validateFds(macro):
        bits = macro.strip().split(' ')
        if len(bits) != 64:
            raise Exception('FDS waveform must have exactly 64 parameters')

        for bit in bits:
            bit = int(bit)
            if bit < 0:
                raise Exception('FDS waveform parameter cannot be less than 0')

            if bit > 63:
                raise Exception('FDS waveform parameter cannot be greater than 63')

        return macro

    @staticmethod
    def maxBufferFromSampleLength(sample_length):
        map = {
            32: 3,
            28: 3,
            24: 4,
            20: 5,
            16: 7,
            12: 9,
             8: 13,
             4: 32
        }

        return map[sample_length]

    @staticmethod
    def getBufferForWaveform(waveform):
        return Instrument.N106_buffers[waveform]

    @staticmethod
    def getN106Buffer(waveform):
        waveform = waveform.strip()
        bits = waveform.split(' ')

        max_allowed_buffer = Instrument.maxBufferFromSampleLength(len(bits))
        buffer = Instrument.getBufferForWaveform(waveform)

        if buffer is None:
            return '00'

        if buffer > max_allowed_buffer:
            raise Exception('buffer value cannot be greater than: ' + str(max_allowed_buffer) + ' for ' + str(len(bits)) + ' samples')

        if buffer < 10:
            buffer = '0' + str(buffer)

        return str(buffer)

    def start(self, whistle):
        start = ''
        if hasattr(self, 'timbre'):
            last_timbre = whistle.getDataForVoice(whistle.current_voices[0], 'timbre')
            new_timbre = self.getTimbreMacro()

            if new_timbre != last_timbre:
                whistle.setDataForVoices(whistle.current_voices, 'timbre', new_timbre)
                start += new_timbre + ' '

        if hasattr(self, 'volume'):
            last_volume = whistle.getDataForVoice(whistle.current_voices[0], 'volume')
            new_volume = self.getVolumeMacro()

            if new_volume != last_volume:
                whistle.setDataForVoices(whistle.current_voices, 'volume', new_volume)
                start += new_volume + ' '

        if hasattr(self, 'pitch'):
            last_pitch = whistle.getDataForVoice(whistle.current_voices[0], 'pitch')
            new_pitch = self.getPitchMacro()

            if new_pitch != last_pitch:
                whistle.setDataForVoices(whistle.current_voices, 'pitch', new_pitch)
                start += new_pitch + ' '

        if hasattr(self, 'arpeggio'):
            last_arpeggio = whistle.getDataForVoice(whistle.current_voices[0], 'arpeggio')
            new_arpeggio = self.getArpeggioMacro()

            if new_arpeggio != last_arpeggio:
                whistle.setDataForVoices(whistle.current_voices, 'arpeggio', new_arpeggio)
                start += new_arpeggio + ' '

        if hasattr(self, 'vibrato'):
            last_vibrato = whistle.getDataForVoice(whistle.current_voices[0], 'vibrato')
            new_vibrato = self.getVibratoMacro()

            if new_vibrato != last_vibrato:
                whistle.setDataForVoices(whistle.current_voices, 'vibrato', new_vibrato)
                start += new_vibrato + ' '

        if hasattr(self, 'q'):
            last_q = whistle.getDataForVoice(whistle.current_voices[0], 'q')
            new_q = 'q' + self.q

            if new_q != last_q:
                whistle.setDataForVoices(whistle.current_voices, 'q', new_q)
                start += new_q + ' '

        if hasattr(self, 'waveform') and self.getChip() == 'N106':
            last_n106 = whistle.getDataForVoice(whistle.current_voices[0], 'timbre')
            new_n106 = self.getN106Macro()

            if new_n106 != last_n106:
                whistle.setDataForVoices(whistle.current_voices, 'timbre', new_n106)
                start += new_n106 + ' '

        if hasattr(self, 'waveform') and self.getChip() == 'FDS':
            last_fds = whistle.getDataForVoice(whistle.current_voices[0], 'timbre')
            new_fds = self.getFDSMacro()

            if new_fds != last_fds:
                whistle.setDataForVoices(whistle.current_voices, 'timbre', new_fds)
                start += new_fds + ' '

        return start

    def end(self, whistle):
        end = ''
        if hasattr(self, 'pitch'):
            whistle.setDataForVoices(whistle.current_voices, 'pitch', None)
            end += 'EPOF '

        if hasattr(self, 'arpeggio'):
            whistle.setDataForVoices(whistle.current_voices, 'arpeggio', None)
            end += 'ENOF '

        if hasattr(self, 'vibrato'):
            whistle.setDataForVoices(whistle.current_voices, 'vibrato', None)
            end += 'MPOF '

        return end


class MagicMacro(object):
    def __init__(self, macro):
        # print "creating object with macro", macro
        self.macro = macro
        self.new_macro = macro

    def repeat(self, count):
        self.new_macro = ((' ' + self.new_macro) * 2).replace('  ', ' ')

    def step(self, rate):
        first = int(self.macro.split(' ')[0])
        last = int(self.macro.split(' ').pop())
        self.new_macro = ' '.join(self.getMagicSteps(first, last, float(rate)))

    def getMagicSteps(self, first, last, rate):
        values = []
        start = first
        if first > last:
            while start >= last:
                values.append(str(int(math.floor(start))))
                start -= abs(rate)

            return values

        while start <= last:
            values.append(str(int(math.floor(start))))
            start += rate

        return values

    def processMagicSteps(self, macro):
        groups = macro.split(' ')

        values = []
        for group in groups:
            if not '..' in group:
                values.append(group)
                continue

            match = re.match(r'(\d+)(\((\+|\-)?(\.?\d+(\.\d+)?)\))?..(\d+)', group)
            if not match:
                values.append(group)
                continue

            first = int(match.group(1))
            last = int(match.group(6))
            rate = float(match.group(4)) if match.group(4) else 1

            values += self.getMagicSteps(first, last, rate)

        return ' '.join(values)

    def __str__(self):
        macro = self.processMagicSteps(self.new_macro)
        return macro
