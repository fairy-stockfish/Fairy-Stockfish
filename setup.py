# -*- coding: utf-8 -*-

from setuptools import setup, Extension
from glob import glob
import platform
import io
import os


if platform.python_compiler().startswith("MSC"):
    args = ["/std:c++17"]
else:
    args = ["-std=c++17", "-flto", "-Wno-date-time"]

args.extend(["-DLARGEBOARDS", "-DPRECOMPUTED_MAGICS", "-DNNUE_EMBEDDING_OFF"])

if "64bit" in platform.architecture():
    args.append("-DIS_64BIT")

CLASSIFIERS = [
    "Development Status :: 3 - Alpha",
    "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
    "Programming Language :: Python :: 3",
    "Operating System :: OS Independent",
]

with io.open("README.md", "r", encoding="utf8") as fh:
    long_description = fh.read().strip()

sources = glob("src/*.cpp") + glob("src/syzygy/*.cpp") + glob("src/nnue/*.cpp") + glob("src/nnue/features/*.cpp")
ffish_source_file = os.path.normcase("src/ffishjs.cpp")
try:
    sources.remove(ffish_source_file)
except ValueError:
    print(f"ffish_source_file {ffish_source_file} was not found in sources {sources}.")

pyffish_module = Extension(
    "pyffish",
    sources=sources,
    extra_compile_args=args)

setup(name="pyffish", version="0.0.52",
      description="Fairy-Stockfish Python wrapper",
      long_description=long_description,
      long_description_content_type="text/markdown",
      author="Bajusz Tamás",
      author_email="gbtami@gmail.com",
      license="GPL3",
      classifiers=CLASSIFIERS,
      url="https://github.com/gbtami/Fairy-Stockfish",
      python_requires=">=2.7,!=3.0.*,!=3.1.*,!=3.2.*,!=3.3.*",
      ext_modules=[pyffish_module]
      )
