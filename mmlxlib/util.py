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
