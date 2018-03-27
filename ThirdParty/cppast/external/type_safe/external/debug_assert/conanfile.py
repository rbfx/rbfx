from conans import ConanFile
from conans.tools import download, untargz
import os


class DebugAssert(ConanFile):
    name = 'debug_assert'
    url  = 'https://foonathan.github.io/blog/2016/09/16/assertions.html'
    version = '1.3.1'
    exports = '*.hpp'
    generators = 'cmake'


    def source(self):
        tar = 'debug_assert-{}.tar.gz'.format(self.version)
        url = 'https://github.com/foonathan/debug_assert/archive/v{}.tar.gz'.format(self.version)

        download(url, tar)
        untargz(tar)

    def package(self):
        srcdir = 'debug_assert-{}'.format(self.version)

        # The header is packaged twice: At include/ (for unqualified #include
        # directives) and include/debug_assert/
        self.copy('debug_assert.hpp', src=srcdir, dst=os.path.join('include', 'debug_assert'))
        self.copy('debug_assert.hpp', src=srcdir, dst='include')
