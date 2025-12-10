#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "rhythmic_delivery.h"

namespace py = pybind11;

PYBIND11_MODULE(rhythmic_delivery, m) {
    m.doc() = "Rhythmic delivery solver (C++/pybind11)";

py::class_<DeliveryResult>(m, "DeliveryResult")
        .def(py::init<>())
        .def(py::init<const Vec&, const Vec&, bool>(),
             py::arg("x"), py::arg("V"), py::arg("ok"))
        .def_readonly("x", &DeliveryResult::x)
        .def_readonly("V", &DeliveryResult::V)
        .def_readonly("ok", &DeliveryResult::ok);

    py::class_<UniformityIterResult, DeliveryResult>(m, "UniformityIterResult")
        .def(py::init<>()) 
        .def(py::init<const Vec&, const Vec&, bool, double, int, int>(),
             py::arg("x"), py::arg("V"), py::arg("ok"),
             py::arg("Mp"), py::arg("maxIter"), py::arg("iters"))
        .def_readonly("Mp", &UniformityIterResult::Mp)
        .def_readonly("maxIter", &UniformityIterResult::maxIter)
        .def_readonly("iters", &UniformityIterResult::iters);


    m.def("solve_uniform_pg", &solve_rhythmic_delivery_uniform_pg,
            py::arg("p"), py::arg("V0"), py::arg("minV"), py::arg("maxV"));
    
    m.def("solve_direct", &solve_rhythmic_delivery_bounds_direct,
        py::arg("p"), py::arg("V0"), py::arg("minV"), py::arg("maxV"));
}
