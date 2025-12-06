#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "rhythmic_delivery.h"

namespace py = pybind11;

PYBIND11_MODULE(rhythmic_delivery, m) {
    m.doc() = "Rhythmic delivery solver (C++/pybind11)";

    py::class_<RhythmicResult>(m, "RhythmicResult")
        .def_readonly("x", &RhythmicResult::x)
        .def_readonly("V", &RhythmicResult::V)
        .def_readonly("Mp", &RhythmicResult::Mp)
        .def_readonly("ok", &RhythmicResult::ok);

    m.def("solve", &solve_rhythmic_delivery,
          py::arg("p"), py::arg("V0"), py::arg("minV"), py::arg("maxV"));
}
