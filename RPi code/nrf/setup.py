"""A setuptools based setup module.
See:
https://packaging.python.org/guides/distributing-packages-using-setuptools/
https://github.com/pypa/sampleproject
"""

# Always prefer setuptools over distutils
from setuptools import setup, find_packages
from os import path
import os
# io.open is needed for projects that support Python 2.7
# It ensures open() defaults to text mode with universal newlines,
# and accepts an argument to specify the text encoding
# Python 3 only projects can skip this import
from io import open

here = path.abspath(path.dirname(__file__))

# Get the long description from the README file
# with open(path.join(here, 'README.md'), encoding='utf-8') as f:
#     long_description = f.read()

# ADD TO PATH - MODIFIED
bashrc_path = os.path.expanduser("~/.bashrc")
bin_path = os.path.expanduser("~/.local/bin")
export_cmd = f'export PATH=$PATH:{bin_path}'

# Check if PATH entry already exists
with open(bashrc_path, 'r') as file:
    bashrc_content = file.read()
    if export_cmd not in bashrc_content:
        # Add export command to .bashrc
        with open(bashrc_path, 'a') as bashrc_file:
            bashrc_file.write(f'\n# Added by setup.py\n{export_cmd}\n')

setup(
    name='nrf',
    # use_scm_version=True,
    # setup_requires=["setuptools_scm"],
    version="4.1.0",
    description='Python libraries for nRF Sniffer',
    # long_description=long_description,
    # long_description_content_type='text/markdown',
    url='https://github.com/embedded-community/nrf-sniffer-cli',
    author='Matias Karhumaa',
    author_email='matias.karhumaa@gmail.com',
    # Classifiers help users find your project by categorizing it.
    # For a list of valid classifiers, see https://pypi.org/classifiers/
    classifiers=[  # Optional
        'Development Status :: 3 - Alpha',
        # Indicate who your project is intended for
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Build Tools',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
    keywords="ble sniffer bluetooth",
    packages=find_packages(exclude=['contrib', 'docs', 'tests']),  # Required
    entry_points={
        'console_scripts': [
            'nrf = SnifferAPI.cli:main'
        ]
    },
    python_requires='>=3.7, <=3.12',
    install_requires=[
        'pyserial>=3.4',
        'psutil'
    ],
    extras_require={
        'dev': ['pytest', 'pytest-cov', 'pylint', 'coverage', 'mock']
    }
)