from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "calc_module",
        [
            "lib/src/bindings.cpp",
            "lib/src/aux_module.cpp",
            "lib/src/rhythmic_delivery.cpp",
            "lib/src/pcplp.cpp",
        ],
        include_dirs=["lib/include"],
        cxx_std=17,
    )
]

setup(
    name="calc-module",
    version="0.1.0",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
