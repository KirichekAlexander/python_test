#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "rhythmic_delivery.h"
#include "pcplp.h"
#include "brigade_routing.h"

namespace py = pybind11;

PYBIND11_MODULE(calc_module, m) {
    m.doc() =  "Calculation module (C++/pybind11)";

    py::class_<DeliveryResult>(m, "DeliveryResult")
        .def(py::init<>())
        .def(py::init<const Vecr&, const Vecr&, bool>(),
             py::arg("x"), py::arg("V"), py::arg("ok"))
        .def_readonly("x", &DeliveryResult::x)
        .def_readonly("V", &DeliveryResult::V)
        .def_readonly("ok", &DeliveryResult::ok);

    py::class_<UniformityIterResult, DeliveryResult>(m, "UniformityIterResult")
        .def(py::init<>()) 
        .def(py::init<const Vecr&, const Vecr&, bool, double, int, int>(),
             py::arg("x"), py::arg("V"), py::arg("ok"),
             py::arg("Mp"), py::arg("maxIter"), py::arg("iters"))
        .def_readonly("Mp", &UniformityIterResult::Mp)
        .def_readonly("maxIter", &UniformityIterResult::maxIter)
        .def_readonly("iters", &UniformityIterResult::iters);


    m.def("solve_uniform_pg", &solve_rhythmic_delivery_uniform_pg,
            py::arg("p"), py::arg("V0"), py::arg("minV"), py::arg("maxV"));
    
    m.def("solve_direct", &solve_rhythmic_delivery_bounds_direct,
        py::arg("p"), py::arg("V0"), py::arg("minV"), py::arg("maxV"));


    py::class_<Schedule>(m, "Schedule")
        .def(py::init<>())
        .def_readonly("start", &Schedule::start)
        .def_readonly("finish", &Schedule::finish)
        .def_readonly("cmax", &Schedule::cmax);

     m.def("solve_pcplp", &solve_PCPLP,
          py::arg("N"), py::arg("M"),
          py::arg("dur"), py::arg("rel"), py::arg("cap"), py::arg("demands"), py::arg("preds"));

    
    py::class_<RoadEdge>(m, "RoadEdge")
        .def(py::init([](std::string src, std::string dst, double km){
            RoadEdge e;
            e.from = std::move(src);
            e.to   = std::move(dst);
            e.km   = km;
            return e;
        }), py::arg("src"), py::arg("dst"), py::arg("km"))
        .def_readwrite("src", &RoadEdge::from)
        .def_readwrite("dst",   &RoadEdge::to)
        .def_readwrite("km",   &RoadEdge::km);

    py::class_<WorkerInput>(m, "WorkerInput")
        .def(py::init([](std::string name, std::string city, std::vector<int> skills){
            WorkerInput w;
            w.name = std::move(name);
            w.city = std::move(city);
            w.skills = std::move(skills);
            return w;
        }), py::arg("name"), py::arg("city"), py::arg("skills"))
        .def_readwrite("name",   &WorkerInput::name)
        .def_readwrite("city",   &WorkerInput::city)
        .def_readwrite("skills", &WorkerInput::skills);

    py::class_<RequestInput>(m, "RequestInput")
        .def(py::init([](int id, std::string city, std::vector<int> tasks){
            RequestInput r;
            r.id = id;
            r.city = std::move(city);
            r.tasks = std::move(tasks);
            return r;
        }), py::arg("id"), py::arg("city"), py::arg("tasks"))
        .def_readwrite("id",    &RequestInput::id)
        .def_readwrite("city",  &RequestInput::city)
        .def_readwrite("tasks", &RequestInput::tasks);

    py::class_<Params>(m, "CrewParams")
        .def(py::init<>())
        .def_readwrite("speed_kmph", &Params::speed_kmph)
        .def_readwrite("p1", &Params::p1)
        .def_readwrite("p2", &Params::p2)
        .def_readwrite("p3", &Params::p3)
        .def_readwrite("p4", &Params::p4);

    py::class_<RequestSolution>(m, "RequestSolution")
        .def(py::init<>())
        .def_readonly("requestId",  &RequestSolution::requestId)
        .def_readonly("feasible",   &RequestSolution::feasible)
        .def_readonly("team",       &RequestSolution::team)
        .def_readonly("requestCity",  &RequestSolution::requestCity)
        .def_readonly("requestTasks", &RequestSolution::requestTasks)
        .def_readonly("teamCity",     &RequestSolution::teamCity)
        .def_readonly("teamNames",    &RequestSolution::teamNames)
        .def_readonly("assignment", &RequestSolution::assignment)
        .def_readonly("lc",         &RequestSolution::lc)
        .def_readonly("startTime",  &RequestSolution::startTime)
        .def_readonly("finishTime", &RequestSolution::finishTime)
        .def_readonly("distKm",     &RequestSolution::distKm)
        .def_readonly("t2Hours",    &RequestSolution::t2Hours);

    py::class_<Solution>(m, "CrewSolution")
        .def(py::init<>())
        .def_readonly("perRequest", &Solution::perRequest);

    m.def("solve_crew_routing", &solve_with_roads,
          py::arg("requests"),
          py::arg("workers"),
          py::arg("tau"),
          py::arg("roads"),
          py::arg("params"),
          py::arg("order") = Veci{},
          py::arg("dynamicW") = false);

    m.def("solve_crew_routing_ga", &solve_with_roads_full_ga,
          py::arg("requests"),
          py::arg("workers"),
          py::arg("tau"),
          py::arg("roads"),
          py::arg("params"),
          py::arg("order") = Veci{},
          py::arg("dynamicW") = false);

    m.def("save_crew_routing_solution_csv", &save_solution_csv,
      py::arg("filename"), py::arg("solution"));

}
