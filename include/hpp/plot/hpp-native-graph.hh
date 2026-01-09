// BSD 2-Clause License

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

#ifndef HPP_PLOT_HPP_NATIVE_GRAPH_HH
#define HPP_PLOT_HPP_NATIVE_GRAPH_HH

#include <QPushButton>
#include <hpp/manipulation/graph/graph.hh>
#include <hpp/plot/graph-widget.hh>

namespace hpp {
namespace plot {

/// \brief Constraint graph viewer using native C++ API
///
/// This widget displays constraint graphs from hpp-manipulation without
/// requiring CORBA. It reads graph structure directly from the C++ Graph
/// object using the same visualization style as HppManipulationGraphWidget.
class HppNativeGraphWidget : public GraphWidget {
  Q_OBJECT

 public:
  /// \brief Constructor
  /// \param graph Shared pointer to manipulation graph (can be nullptr)
  /// \param parent Parent widget
  HppNativeGraphWidget(hpp::manipulation::graph::GraphPtr_t graph = nullptr,
                       QWidget* parent = nullptr);

  /// \brief Destructor
  ~HppNativeGraphWidget();

  /// \brief Set graph object and refresh display
  /// \param graph Shared pointer to manipulation graph
  void setGraph(hpp::manipulation::graph::GraphPtr_t graph);

  /// \brief Get the graph name
  const std::string& graphName() const { return graphName_; }

  /// \brief Get currently selected element ID
  /// \param[out] id The ID of the selected element
  /// \return true if an element is selected, false otherwise
  bool selectionID(std::size_t& id);

  /// \brief Highlight a specific node (e.g., for current configuration)
  /// \param nodeId The ID of the node to highlight, or -1 to clear
  void highlightNode(long nodeId);

  /// \brief Highlight a specific edge
  /// \param edgeId The ID of the edge to highlight, or -1 to clear
  void highlightEdge(long edgeId);

 public slots:
  /// \brief Display detailed state constraints in the constraint panel
  void displayStateConstraints(std::size_t id);

  /// \brief Display detailed edge constraints in the constraint panel
  void displayEdgeConstraints(std::size_t id);

  /// \brief Display edge target constraints in the constraint panel
  void displayEdgeTargetConstraints(std::size_t id);

 protected:
  /// \brief Fill scene from Graph object
  /// Reads nodes and edges directly from the C++ graph structure
  void fillScene() override;

 protected slots:
  virtual void nodeContextMenu(QGVNode* node);
  virtual void nodeDoubleClick(QGVNode* node);
  virtual void edgeContextMenu(QGVEdge* edge);
  virtual void edgeDoubleClick(QGVEdge* edge);
  void selectionChanged();

 private:
  /// Get constraint names for a graph component
  QString getConstraints(
      const hpp::manipulation::graph::GraphComponentPtr_t& component);

  /// Get detailed constraint string (with solver info) for a state
  QString getDetailedStateConstraints(std::size_t stateId);

  /// Get detailed constraint string for an edge
  QString getDetailedEdgeConstraints(std::size_t edgeId);

  /// Get detailed constraint string for edge target
  QString getDetailedEdgeTargetConstraints(std::size_t edgeId);

  /// Update edge visual style based on weight
  void updateEdgeStyle(QGVEdge* edge, long weight);

  hpp::manipulation::graph::GraphPtr_t graph_;
  std::string graphName_;

  /// Graph-level information
  struct GraphInfo {
    std::size_t id;
    QString constraintStr;
  } graphInfo_;

  /// Node information
  struct NodeInfo {
    std::size_t id;
    QString name;
    QString constraintStr;
    QGVNode* node;
    bool isWaypoint;
  };

  /// Edge information
  struct EdgeInfo {
    std::size_t id;
    QString name;
    QString constraintStr;
    QString containingStateName;
    long weight;
    bool isShort;
    std::size_t nbWaypoints;  // Number of waypoints (0 for regular edges)
    QGVEdge* edge;
  };

  QMap<QGVNode*, NodeInfo> nodeInfos_;
  QMap<QGVEdge*, EdgeInfo> edgeInfos_;
  QMap<std::size_t, QGVNode*> nodes_;
  QMap<std::size_t, QGVEdge*> edges_;

  /// UI buttons
  QPushButton* showWaypoints_;

  /// Currently selected element ID (-1 if none)
  long currentId_;

  /// Currently highlighted node/edge IDs (-1 if none)
  long highlightedNodeId_;
  long highlightedEdgeId_;
};

}  // namespace plot
}  // namespace hpp

#endif  // HPP_PLOT_HPP_NATIVE_GRAPH_HH
