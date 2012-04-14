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
import re
import math


class MagicMacro(object):
    def __init__(self, macro):
        # print "creating object with macro", macro
        self.macro = macro
        self.step_size = None
        self.repeat_count = None
        self.curve_type = None

    def repeat(self, count):
        self.repeat_count = int(count)

    def step(self, size):
        self.step_size = float(size)

    def curve(self, type="+"):
        self.curve_type = type

    def processRepeats(self, macro):
        return ((' ' + macro) * int(self.repeat_count)).replace('  ', ' ')

    def processSteps(self, macro):
        first = int(macro.split(' ')[0])
        last = int(macro.split(' ').pop())
        return ' '.join(self.getMagicSteps(first, last, self.step_size))

    # t: current time, b: begInnIng value, c: change In value, d: duration
    def easeIn(self, t, b, c, d):
        if t == 0:
            return b

        return math.pow(2, 10 * (t / d - 1)) + b

    def easeInQuad(self, t, b, c, d):
        t = float(t) / float(d)
        return c * t * t + b

    def processCurve(self, macro):
        return macro
        # begin = 0
        # finish = 15
        # change = finish - begin
        # duration = 15
        # time = 0

        # while time <= duration:
        #     pos = self.easeInQuad(time, begin, change, duration)
        #     print math.floor(pos)

        #     time += 1
        #     # print time

        # return macro
        # first = int(macro.split(' ')[0])
        # last = int(macro.split(' ').pop())
        # total_steps = abs(last - first + 1)

        # # xs = [1, int(total_steps / 2), total_steps]
        # # print self.curve_type
        # # cushion = (last - first) / 15
        # # ys = [first, last if self.curve_type == '-' else first + cushion, last]
        # # print xs, ys
        # xs = [1, 8, 13]
        # ys = [3, 3, 15]

        # equation = numpy.poly1d(numpy.polyfit(xs, ys, deg=2))

        # step_size = self.step_size or 1
        # values = []
        # i = 1
        # while i <= total_steps:
        #     print '(' + str(i) + ', ' + str(int(round(equation(i)))) + ')'
        #     values.append(str(int(round(equation(i)))))
        #     i += step_size

        # return " ".join(values)

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
        macro = self.macro

        if self.curve_type is not None:
            return self.processCurve(macro)

        if self.step_size is not None:
            macro = self.processSteps(macro)

        if self.repeat_count is not None:
            macro = self.processRepeats(macro)

        macro = self.processMagicSteps(macro)
        return macro
