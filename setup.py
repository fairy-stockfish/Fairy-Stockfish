# -*- coding: utf-8 -*-

from setuptools import setup, Extension
from glob import glob
import platform

args = ["-Wno-date-time", "-flto", "-DLARGEBOARDS", "-DPRECOMPUTED_MAGICS"]

if "64bit" in platform.architecture():
    args.append("-DIS_64BIT")

CLASSIFIERS = [
    "Development Status :: 3 - Alpha",
    "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
    "Programming Language :: Python :: 3",
    "Operating System :: OS Independent",
]

with open("Readme.md", "r") as fh:
    long_description = fh.read().strip()

pyffish_module = Extension(
    "pyffish",
    sources=glob("src/*.cpp") + glob("src/syzygy/*.cpp"),
    extra_compile_args=args)

setup(name="pyffish", version="0.0.21",
      description="Fairy-Stockfish Python wrapper",
      long_description=long_description,
      long_description_content_type="text/markdown",
      author="Bajusz Tamás",
      author_email="gbtami@gmail.com",
      license="GPL3",
      classifiers=CLASSIFIERS,
      url="https://github.com/gbtami/Fairy-Stockfish",
      ext_modules=[pyffish_module]
      )
