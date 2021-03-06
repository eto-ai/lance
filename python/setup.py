#!/usr/bin/env python3
#
# Copyright 2022 Lance Developers
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from pathlib import Path
from setuptools import Extension, find_packages, setup

import numpy as np
import pyarrow as pa
from Cython.Build import cythonize

arrow_includes = pa.get_include()
arrow_library_dirs = pa.get_library_dirs()
numpy_includes = np.get_include()

# TODO allow for custom liblance directory
lance_cpp = Path(__file__).resolve().parent.parent / "cpp"
lance_includes = str(lance_cpp / "include")
lance_libs = str(lance_cpp / "build")

extensions = [
    Extension(
        "lance.lib",
        ["lance/_lib.pyx"],
        include_dirs=[lance_includes, arrow_includes, numpy_includes],
        libraries=["lance"],
        library_dirs=[lance_libs] + arrow_library_dirs,
        language="c++",
        extra_compile_args=["-Wall", "-std=c++20", "-O3"],
        extra_link_args=["-Wl,-rpath", lance_libs, "-larrow_python"],
    )
]

# The information here can also be placed in setup.cfg - better separation of
# logic and declaration, and simpler if you include description/version in a file.
setup(
    name="pylance",
    version="0.0.1",
    author="Lance Developers",
    author_email="contact@eto.ai",
    description="Python extension for lance",
    long_description="",
    ext_modules=cythonize(extensions, language_level="3"),
    zip_safe=False,
    install_requires=["pyarrow"],
    extras_require={"test": ["pytest>=6.0", "pandas", "duckdb"]},
    python_requires=">=3.8",
    packages=find_packages(),
)
