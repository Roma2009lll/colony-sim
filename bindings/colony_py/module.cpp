#include <pybind11/pybind11.h>

int ping() {
    return 42;
}

PYBIND11_MODULE(colony_py, m) {
    m.doc() = "Colony Simulation C++ Python Bindings";
    m.def("ping", &ping, "Smoke test function returning 42");
}