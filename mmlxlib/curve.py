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

# TERMS OF USE - EASING EQUATIONS
#
# Open source under the BSD License.
#
# Copyright 2001 Robert Penner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this list of
# conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice, this list
# of conditions and the following disclaimer in the documentation and/or other materials
# provided with the distribution.
#
# Neither the name of the author nor the names of contributors may be used to endorse
# or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

#                              .::::.
#                            .::::::::.
#                            :::::::::::
#                            ':::::::::::..
#                             :::::::::::::::'
#                              ':::::::::::.
#                                .::::::::::::::'
#                              .:::::::::::...
#                             ::::::::::::::''
#                 .:::.       '::::::::''::::
#               .::::::::.      ':::::'  '::::
#              .::::':::::::.    :::::    '::::.
#            .:::::' ':::::::::. :::::      ':::.
#          .:::::'     ':::::::::.:::::       '::.
#        .::::''         '::::::::::::::       '::.
#       .::''              '::::::::::::         :::...
#    ..::::                  ':::::::::'        .:' ''''
# ..''''':'                    ':::::.'


class Curve(object):
    def __init__(self, begin, end, duration):
        self.begin = begin
        self.end = end
        self.change = end - begin
        self.duration = duration

    # ==============================================================
    #                           EASE IN QUAD
    # ==============================================================
    #                                                             *
    #                                                           *
    #                                                         *
    #                                                       *
    #                                                     *
    #                                                   *
    #                                                 *
    #                                             * *
    #                                           *
    #                                       * *
    #                                     *
    #                                 * *
    #                             * *
    #                       * * *
    #                 * * *
    # * * * * * * * *
    # ==============================================================
    def easeInQuad(self, t, b, c, d):
        t = t / d
        return c * t * t + b

    # ==============================================================
    #                           EASE OUT QUAD
    # ==============================================================
    #                                                             *
    #                                               * * * * * * *
    #                                         * * *
    #                                   * * *
    #                               * *
    #                           * *
    #                         *
    #                     * *
    #                   *
    #               * *
    #             *
    #           *
    #         *
    #       *
    #     *
    # * *
    # ==============================================================
    def easeOutQuad(self, t, b, c, d):
        t = t / d
        return -c * t * (t - 2) + b

    # ==============================================================
    #                         EASE IN OUT QUAD
    # ==============================================================
    #                                                             *
    #                                                   * * * * *
    #                                               * *
    #                                           * *
    #                                         *
    #                                     * *
    #                                   *
    #                                 *
    #                               *
    #                             *
    #                           *
    #                       * *
    #                     *
    #                 * *
    #             * *
    # * * * * * *
    # ==============================================================
    def easeInOutQuad(self, t, b, c, d):
        t = t / (d / 2)
        if t < 1:
            return c / 2 * t * t + b

        t -= 1
        return -c / 2 * (t * (t - 2) - 1) + b

    # ==============================================================
    #                           EASE IN CUBIC
    # ==============================================================
    #                                                             *
    #
    #                                                           *
    #                                                         *
    #
    #                                                       *
    #                                                     *
    #                                                   *
    #                                                 *
    #                                               *
    #                                           * *
    #                                         *
    #                                     * *
    #                                 * *
    #                           * * *
    # * * * * * * * * * * * * *
    # ==============================================================
    def easeInCubic(self, t, b, c, d):
        t = t / d
        return c * t * t * t + b

    # ==============================================================
    #                         EASE OUT CUBIC
    # ==============================================================
    #                                                             *
    #                                     * * * * * * * * * * * *
    #                               * * *
    #                           * *
    #                       * *
    #                     *
    #                 * *
    #               *
    #             *
    #           *
    #         *
    #       *
    #
    #     *
    #   *
    # *
    # ==============================================================
    def easeOutCubic(self, t, b, c, d):
        t = t / d - 1
        return c * (t * t * t + 1) + b

    # ==============================================================
    #                         EASE IN OUT CUBIC
    # ==============================================================
    #                                                             *
    #                                               * * * * * * *
    #                                           * *
    #                                       * *
    #                                     *
    #                                   *
    #
    #                                 *
    #                               *
    #                             *
    #
    #                           *
    #                         *
    #                     * *
    #                 * *
    # * * * * * * * *
    # ==============================================================
    def easeInOutCubic(self, t, b, c, d):
        t = t / (d / 2)
        if t < 1:
            return c / 2 * t * t * t + b

        t -= 2
        return c / 2 * (t * t * t + 2) + b

    # ==============================================================
    #                          EASE IN QUART
    # ==============================================================
    #                                                             *
    #
    #                                                           *
    #
    #                                                         *
    #
    #                                                       *
    #                                                     *
    #                                                   *
    #                                                 *
    #                                               *
    #                                             *
    #                                           *
    #                                       * *
    #                                 * * *
    # * * * * * * * * * * * * * * * *
    # ==============================================================
    def easeInQuart(self, t, b, c, d):
        t = t / d
        return c * t * t * t * t + b

    # ==============================================================
    #                         EASE OUT QUART
    # ==============================================================
    #                                                             *
    #                               * * * * * * * * * * * * * * *
    #                         * * *
    #                     * *
    #                   *
    #                 *
    #               *
    #             *
    #           *
    #         *
    #       *
    #
    #     *
    #
    #   *
    # *
    # ==============================================================
    def easeOutQuart(self, t, b, c, d):
        t = t / d - 1
        return -c * (t * t * t * t - 1) + b

    # ==============================================================
    #                        EASE IN OUT QUART
    # ==============================================================
    #                                                             *
    #                                           * * * * * * * * *
    #                                         *
    #                                       *
    #                                     *
    #                                   *
    #                                 *
    #
    #                               *
    #
    #                             *
    #                           *
    #                         *
    #                       *
    #                     *
    # * * * * * * * * * *
    # ==============================================================
    def easeInOutQuart(self, t, b, c, d):
        t = t / (d / 2)
        if t < 1:
            return c / 2 * t * t * t * t + b

        t -= 2
        return -c / 2 * (t * t * t * t - 2) + b

    def render(self, curve_type):
        time = 0
        positions = []
        while time <= self.duration:
            pos = getattr(self, curve_type)(time, self.begin, self.change, self.duration)
            positions.append(str(int(pos)))
            time += 1

        return ' '.join(positions)
