from setuptools import Extension
from Cython.Build import cythonize
from os.path import dirname, join

path = dirname(__file__)

cythonize(Extension("_rssbotlib", [join(path, "_rssbotlib.pyx")]), compiler_directives={'language_level': "3"})
