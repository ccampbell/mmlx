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
import operator, os

class Util(object):

    @staticmethod
    def openFile(path):
        file = open(path, "r")
        content = file.read()
        file.close()
        return content

    @staticmethod
    def writeFile(path, content):
        file = open(path, "w")
        file.write(content)
        file.close()

    @staticmethod
    def sortDictionary(dictionary):
        return sorted(dictionary.iteritems(), key=operator.itemgetter(1))

    @staticmethod
    def isFileOrDirectory(path):
        return os.path.isfile(path) or os.path.isdir(path)
