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

class Instrument(object):

    def __init__(self, data):
        for key in data:
            setattr(self, key, data[key])

        if hasattr(self, "adsr"):
            self.volume = self.getVolumeFromADSR(self.adsr)

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

    @staticmethod
    def reset(counter = 0):
        counter = int(counter)

        Instrument.counters = {
            'timbre': counter,
            'volume': counter,
            'pitch': counter,
            'arpeggio': counter,
            'vibrato': counter
        }

        Instrument.timbres = {}
        Instrument.volumes = {}
        Instrument.pitches = {}
        Instrument.arpeggios = {}
        Instrument.vibratos = {}

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

    @staticmethod
    def hasBeenUsed():
        macros = ["timbres", "volumes", "pitches", "arpeggios", "vibratos"]
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

        return macros

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
