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
import os, glob, time, sys

class Listener(object):

    def __init__(self, logger=None):
        self.watching = False
        self.callback = None
        self.first_run = True
        self.file_list = {}
        self.logger = logger

    def getFilesFromDir(self, path, extension = ""):
        path = path + "/*"

        if not extension == "":
            path = path + "." + extension.lstrip(".")

        return glob.glob(path)

    def onChange(self, callback):
        self.callback = callback

    def process(self, start, end, is_dir = None):
        try:
            if is_dir is None:
                is_dir = os.path.isdir(start)

            if is_dir:
                files = self.getFilesFromDir(start, "mmlx")
                output_file = None
            else:
                files = [start]
                output_file = end

            initial_output_file = output_file

            for file in files:
                output_file = initial_output_file

                filename = os.path.basename(file)
                if filename.startswith('_'):
                    continue

                last_changed = os.stat(file).st_mtime
                output_file = output_file if output_file is not None else os.path.join(end, filename.replace(".mmlx", ".mml"))

                if not self.file_list.has_key(file):
                    self.file_list[file] = last_changed
                    self.callback(file, output_file)
                    continue

                if last_changed != self.file_list[file]:
                    self.logger.log(self.logger.color("detected change to: ", self.logger.GRAY) + self.logger.color(file, self.logger.UNDERLINE))
                    self.file_list[file] = last_changed
                    self.callback(file, output_file, True)

            if self.first_run:
                self.logger.log('')
                self.first_run = False

        except Exception:
            self.logger.log(self.logger.color('Sorry, an error occured:\n', self.logger.RED))
            import traceback
            lines = traceback.format_exc().splitlines()
            self.logger.log(self.logger.color(lines.pop(), self.logger.RED) + '\n')
            self.logger.log('\n'.join(lines))
            self.logger.log('')

            # continue watching
            if self.watching:
                self.watch(start, end)
                return

            sys.exit(1)

    def watch(self, start, end):
        try:
            self.watching = True
            is_dir = os.path.isdir(start)
            while 1:
                self.process(start, end, is_dir)
                time.sleep(.5)
        except KeyboardInterrupt:
            phrases = [
                'Sayonara!',
                'Goodbye! Have a nice day!',
                'Come back soon!',
                'Sad to see you leaving already!',
                'Hasta la vista, baby'
            ]
            from random import choice
            self.logger.log(self.logger.color('\n' + choice(phrases), self.logger.PINK))
            sys.exit(0)
