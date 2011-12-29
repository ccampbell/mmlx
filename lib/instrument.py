from util import Util

class Instrument(object):

    def __init__(self, data):
        for key in data:
            setattr(self, key, data[key])

    @staticmethod
    def reset():
        Instrument.counters = {
            'timbre': 20,
            'volume': 20,
            'pitch': 20,
            'arpeggio': 20,
            'vibrato': 20
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

    def start(self):
        start = ''
        if hasattr(self, 'timbre'):
            start += self.getTimbreMacro() + ' '

        if hasattr(self, 'volume'):
            start += self.getVolumeMacro() + ' '

        if hasattr(self, 'pitch'):
            start += self.getPitchMacro() + ' '

        if hasattr(self, 'arpeggio'):
            start += self.getArpeggioMacro() + ' '

        if hasattr(self, 'vibrato'):
            start += self.getVibratoMacro() + ' '

        if hasattr(self, 'q'):
            start += 'q' + self.q + ' '

        return start

    def end(self):
        end = ''
        if hasattr(self, 'pitch'):
            end += 'EPOF '

        if hasattr(self, 'arpeggio'):
            end += 'ENOF '

        if hasattr(self, 'vibrato'):
            end += 'MPOF '

        return end
