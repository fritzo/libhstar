import os
import re
import setuptools

# TODO add MANIFEST.in as described in
# http://docs.python.org/2/distutils/sourcedist.html#the-manifest-in-template


version = None
with open(os.path.join('src', '__init__.py')) as f:
    for line in f:
        if re.match("__version__ = '\S+'$", line):
            version = line.split()[-1].strip("'")
assert version, 'could not determine version'

with open('README.md') as f:
    long_description = f.read()

config = {
    'name': 'hstar',
    'version': version,
    'description': 'A virtual machine for extensional lambda-calculus',
    'long_description': long_description,
    'url': 'https://github.com/fritzo/libhstar',
    'author': 'Fritz Obermeyer',
    'maintainer': 'Fritz Obermeyer',
    'maintainer_email': 'fritz.obermeyer@gmail.com',
    'license': 'Apache 2.0',
    'packages': setuptools.find_packages(exclude='src'),
    'setup_requires': ['cffi>=1.0.0'],
    'cffi_modules': ['example_build.py:ffi'],
    'install_requires': ['cffi>=1.0.0'],
}

setuptools.setup(**config)
