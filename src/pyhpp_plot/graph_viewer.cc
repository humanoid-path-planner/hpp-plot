// BSD 2-Clause License
//
// Copyright (c) 2025, hpp-plot
// Authors: Paul Sardin
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:

// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in
//   the documentation and/or other materials provided with the
//   distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

// clang-format off
// Boost.Python must be included before Qt headers to avoid keyword conflicts
// Prevent pre commit from reorganizing includes alphabetically
#include <boost/python.hpp>
#include <boost/python/extract.hpp>

#include <QApplication>
#include <QMainWindow>
// clang-format on

#include <hpp/manipulation/graph/graph.hh>
#include <hpp/plot/hpp-native-graph.hh>

// Workaround for Qt/Python keyword conflict (Python 3.13+)
// Must undef Qt keywords before including Python headers
#undef slots
#undef signals
#undef emit

#include <boost/python.hpp>
#include <boost/python/extract.hpp>

namespace bp = boost::python;

namespace {

using GraphPtr_t = hpp::manipulation::graph::GraphPtr_t;

static const char* GRAPH_CAPSULE_NAME = "hpp.manipulation.graph.GraphPtr";

/// Extract GraphPtr_t from a Python object
/// via the '_get_native_graph()' method which returns a PyCapsule
GraphPtr_t extractGraph(bp::object py_graph) {
  if (PyObject_HasAttrString(py_graph.ptr(), "_get_native_graph")) {
    bp::object capsule_obj = py_graph.attr("_get_native_graph")();
    PyObject* capsule = capsule_obj.ptr();

    // Verify it's a valid capsule with the expected name
    if (PyCapsule_IsValid(capsule, GRAPH_CAPSULE_NAME)) {
      // Extract the GraphPtr_t* from the capsule
      auto* ptr = static_cast<GraphPtr_t*>(
          PyCapsule_GetPointer(capsule, GRAPH_CAPSULE_NAME));
      if (ptr && *ptr) {
        return *ptr;  // Return a copy of the shared_ptr
      }
    }
  }

  bp::extract<GraphPtr_t> direct_extract(py_graph);
  if (direct_extract.check()) {
    return direct_extract();
  }

  throw std::runtime_error(
      "Cannot extract Graph from Python object. "
      "Expected a Graph object from pyhpp.manipulation with "
      "'_get_native_graph()' method.");
}

/// Simple wrapper to allow Python to add menu actions
class MenuActionProxy {
public:
  MenuActionProxy(QMenu* menu) : menu_(menu) {}

  void addAction(const std::string& text, bp::object callback) {
    QAction* action = menu_->addAction(QString::fromStdString(text));
    // Store callback and connect
    QObject::connect(action, &QAction::triggered, [callback]() {
      try {
        callback();
      } catch (const bp::error_already_set&) {
        PyErr_Print();
      }
    });
  }

  void addSeparator() {
    menu_->addSeparator();
  }

private:
  QMenu* menu_;
};

/// This function blocks until the window is closed
void showGraphBlocking(bp::object py_graph) {
  GraphPtr_t graph = extractGraph(py_graph);

  if (!graph) {
    throw std::runtime_error("Graph is null");
  }

  int argc = 1;
  static char app_name[] = "hpp-plot-native";
  char* argv[] = {app_name, nullptr};

  QApplication app(argc, argv);

  // Create main window
  QMainWindow window;
  window.setWindowTitle(
      QString::fromStdString("Constraint Graph: " + graph->name()));
  window.resize(1200, 800);

  // Create widget
  hpp::plot::HppNativeGraphWidget widget(graph, &window);
  widget.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  window.setCentralWidget(&widget);

  // Show and refresh
  window.show();
  widget.updateGraph();

  // Run event loop (blocking)
  app.exec();
}

/// Interactive version with Python callbacks for context menus
void showInteractiveGraph(bp::object py_graph,
                         bp::object node_callback,
                         bp::object edge_callback) {
  GraphPtr_t graph = extractGraph(py_graph);

  if (!graph) {
    throw std::runtime_error("Graph is null");
  }

  int argc = 1;
  static char app_name[] = "hpp-plot-interactive";
  char* argv[] = {app_name, nullptr};

  QApplication app(argc, argv);

  // Create main window
  QMainWindow window;
  window.setWindowTitle(
      QString::fromStdString("Constraint Graph: " + graph->name()));
  window.resize(1200, 800);

  // Create widget
  hpp::plot::HppNativeGraphWidget widget(graph, &window);
  widget.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  window.setCentralWidget(&widget);

  // Connect Qt signals to Python callbacks
  if (!node_callback.is_none()) {
    QObject::connect(&widget, &hpp::plot::HppNativeGraphWidget::nodeContextMenuAboutToShow,
                    [node_callback](std::size_t nodeId, QString nodeName, QMenu* menu) {
      try {
        MenuActionProxy proxy(menu);
        node_callback(nodeId, nodeName.toStdString(), boost::ref(proxy));
      } catch (const bp::error_already_set&) {
        PyErr_Print();
      }
    });
  }

  if (!edge_callback.is_none()) {
    QObject::connect(&widget, &hpp::plot::HppNativeGraphWidget::edgeContextMenuAboutToShow,
                    [edge_callback](std::size_t edgeId, QString edgeName, QMenu* menu) {
      try {
        MenuActionProxy proxy(menu);
        edge_callback(edgeId, edgeName.toStdString(), boost::ref(proxy));
      } catch (const bp::error_already_set&) {
        PyErr_Print();
      }
    });
  }

  // Show and refresh
  window.show();
  widget.updateGraph();

  // Run event loop (blocking)
  app.exec();
}

}  // namespace

BOOST_PYTHON_MODULE(graph_viewer) {
  // Expose MenuActionProxy for adding menu actions from Python
  bp::class_<MenuActionProxy, boost::noncopyable>("MenuActionProxy", bp::no_init)
      .def("addAction", &MenuActionProxy::addAction,
           (bp::arg("text"), bp::arg("callback")),
           "Add an action to the context menu.\n\n"
           "Args:\n"
           "    text: The text label for the menu action\n"
           "    callback: Python callable to invoke when action is triggered\n")
      .def("addSeparator", &MenuActionProxy::addSeparator,
           "Add a separator line to the context menu\n");

  bp::def("show_graph", &showGraphBlocking, bp::arg("graph"),
          "Show constraint graph in a Qt viewer (blocking).\n\n"
          "This function blocks until the viewer window is closed.\n\n"
          "Args:\n"
          "    graph: The Graph object from pyhpp.manipulation\n");

  // Alias for backwards compatibility
  bp::def("show_graph_blocking", &showGraphBlocking, bp::arg("graph"),
          "Alias for show_graph() for backwards compatibility.\n");

  bp::def("show_interactive_graph", &showInteractiveGraph,
          (bp::arg("graph"), bp::arg("node_callback") = bp::object(),
           bp::arg("edge_callback") = bp::object()),
          "Show constraint graph with interactive context menu callbacks.\n\n"
          "This function blocks until the viewer window is closed.\n"
          "Callbacks are invoked when user right-clicks on nodes/edges.\n\n"
          "Args:\n"
          "    graph: The Graph object from pyhpp.manipulation\n"
          "    node_callback: Optional callback(node_id, node_name, menu_proxy) for node menus\n"
          "    edge_callback: Optional callback(edge_id, edge_name, menu_proxy) for edge menus\n");
}
