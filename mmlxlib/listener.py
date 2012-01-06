import os, glob, time

class Listener(object):

    def __init__(self, logger=None):
        self.callback = None
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

    def watch(self, start, end):
        is_dir = os.path.isdir(start)
        while 1:
            self.process(start, end, is_dir)
            time.sleep(.5)
