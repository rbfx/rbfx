
class InterfaceWriter(object):
    def __init__(self, output_path):
        self._output_path_template = output_path + '/_{key}_{subsystem}.i'
        self._fp = {
            'pre': {},
            'post': {},
        }

    def _write(self, key, subsystem, text):
        subsystem = subsystem.lower()
        fp = self._fp[key].get(subsystem)
        if fp is None:
            self._fp[key][subsystem] = fp = open(self._output_path_template.format(key=key, subsystem=subsystem), 'w+')
        fp.write(text)
        fp.write('\n')

    def write_pre(self, subsystem, text):
        self._write('pre', subsystem, text)

    def write_post(self, subsystem, text):
        self._write('post', subsystem, text)

    def close(self):
        for fp_map in self._fp.values():
            for fp in fp_map.values():
                fp.close()
