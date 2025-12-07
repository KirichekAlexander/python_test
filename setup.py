from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension, build_ext

ext_modules = [
    Pybind11Extension(
        "rhythmic_delivery",
        [
            "lib/src/bindings.cpp",
            "lib/src/aux_module.cpp",
            "lib/src/rhythmic_delivery.cpp",
        ],
        include_dirs=["lib/include"],
        cxx_std=11,
    )
]

setup(
    name="rhythmic-delivery",
    version="0.1.0",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)
