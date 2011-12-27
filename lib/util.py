
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
