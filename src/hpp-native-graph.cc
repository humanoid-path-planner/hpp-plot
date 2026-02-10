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

#include "hpp/plot/hpp-native-graph.hh"

#include <QGVEdge.h>
#include <QGVNode.h>

#include <QDebug>
#include <QLayout>
#include <QMenu>
#include <hpp/constraints/differentiable-function.hh>
#include <hpp/constraints/implicit.hh>
#include <hpp/constraints/solver/by-substitution.hh>
#include <hpp/manipulation/graph/edge.hh>
#include <hpp/manipulation/graph/state-selector.hh>
#include <hpp/manipulation/graph/state.hh>
#include <limits>
#include <sstream>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define ESCAPE(q) Qt::escape(q)
#else
#define ESCAPE(q) q.toHtmlEscaped()
#endif

namespace hpp {
namespace plot {

using manipulation::graph::Edge;
using manipulation::graph::EdgePtr_t;
using manipulation::graph::Edges_t;
using manipulation::graph::GraphComponentPtr_t;
using manipulation::graph::GraphPtr_t;
using manipulation::graph::State;
using manipulation::graph::StatePtr_t;
using manipulation::graph::States_t;
using manipulation::graph::StateSelectorPtr_t;
using manipulation::graph::WaypointEdge;
using manipulation::graph::WaypointEdgePtr_t;

HppNativeGraphWidget::HppNativeGraphWidget(GraphPtr_t graph, QWidget* parent)
    : GraphWidget("Manipulation graph", parent),
      graph_(graph),
      showWaypoints_(new QPushButton(QIcon::fromTheme("view-refresh"),
                                     "&Show waypoints", buttonBox_)),
      currentId_(-1),
      highlightedNodeId_(-1),
      highlightedEdgeId_(-1) {
  graphInfo_.id = 0;

  if (graph_) {
    graphName_ = graph_->name();
    graphInfo_.id = graph_->id();
    graphInfo_.constraintStr = getConstraints(graph_);
  }

  // Setup Show Waypoints button
  showWaypoints_->setCheckable(true);
  showWaypoints_->setChecked(false);
  buttonBox_->layout()->addWidget(showWaypoints_);

  // Connect signals
  connect(scene_, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
  connect(showWaypoints_, SIGNAL(clicked()), this, SLOT(updateGraph()));
}

HppNativeGraphWidget::~HppNativeGraphWidget() {}

void HppNativeGraphWidget::setGraph(GraphPtr_t graph) {
  graph_ = graph;
  if (graph_) {
    graphName_ = graph_->name();
    graphInfo_.id = graph_->id();
    graphInfo_.constraintStr = getConstraints(graph_);
  } else {
    graphName_.clear();
    graphInfo_.id = 0;
    graphInfo_.constraintStr.clear();
  }
  updateGraph();
}

bool HppNativeGraphWidget::selectionID(std::size_t& id) {
  if (currentId_ < 0) return false;
  id = static_cast<std::size_t>(currentId_);
  return true;
}

void HppNativeGraphWidget::highlightNode(long nodeId) {
  // Clear previous highlight
  if (highlightedNodeId_ >= 0 &&
      nodes_.contains(static_cast<std::size_t>(highlightedNodeId_))) {
    QGVNode* prevNode = nodes_[static_cast<std::size_t>(highlightedNodeId_)];
    prevNode->setAttribute("fillcolor", "white");
    prevNode->updateLayout();
  }

  highlightedNodeId_ = nodeId;

  // Apply new highlight
  if (nodeId >= 0 && nodes_.contains(static_cast<std::size_t>(nodeId))) {
    QGVNode* node = nodes_[static_cast<std::size_t>(nodeId)];
    node->setAttribute("fillcolor", "green");
    node->updateLayout();
    scene_->update();
  }
}

void HppNativeGraphWidget::highlightEdge(long edgeId) {
  // Clear previous highlight
  if (highlightedEdgeId_ >= 0 &&
      edges_.contains(static_cast<std::size_t>(highlightedEdgeId_))) {
    QGVEdge* prevEdge = edges_[static_cast<std::size_t>(highlightedEdgeId_)];
    prevEdge->setAttribute("color", "");
    prevEdge->updateLayout();
  }

  highlightedEdgeId_ = edgeId;

  // Apply new highlight
  if (edgeId >= 0 && edges_.contains(static_cast<std::size_t>(edgeId))) {
    QGVEdge* edge = edges_[static_cast<std::size_t>(edgeId)];
    edge->setAttribute("color", "green");
    edge->updateLayout();
    scene_->update();
  }
}

void HppNativeGraphWidget::displayStateConstraints(std::size_t id) {
  QString str = getDetailedStateConstraints(id);
  constraintInfo_->setText(str);
}

void HppNativeGraphWidget::displayEdgeConstraints(std::size_t id) {
  QString str = getDetailedEdgeConstraints(id);
  constraintInfo_->setText(str);
}

void HppNativeGraphWidget::displayEdgeTargetConstraints(std::size_t id) {
  QString str = getDetailedEdgeTargetConstraints(id);
  constraintInfo_->setText(str);
}

void HppNativeGraphWidget::fillScene() {
  if (!graph_) {
    qDebug() << "No graph set";
    return;
  }

  try {
    // Set graph attributes
    scene_->setGraphAttribute("label", QString::fromStdString(graph_->name()));
    scene_->setGraphAttribute("splines", "spline");
    scene_->setGraphAttribute("outputorder", "edgesfirst");
    scene_->setGraphAttribute("nodesep", "0.5");
    scene_->setGraphAttribute("esep", "0.8");
    scene_->setGraphAttribute("sep", "1");

    // Node attributes
    scene_->setNodeAttribute("shape", "circle");
    scene_->setNodeAttribute("style", "filled");
    scene_->setNodeAttribute("fillcolor", "white");

    // Clear existing maps
    nodeInfos_.clear();
    edgeInfos_.clear();
    nodes_.clear();
    edges_.clear();

    // Get state selector
    StateSelectorPtr_t selector = graph_->stateSelector();
    if (!selector) {
      qDebug() << "No state selector";
      return;
    }

    // Get all states
    States_t states = selector->getStates();
    bool hideWaypoints = !showWaypoints_->isChecked();

    QMap<std::size_t, bool> nodeIsWaypoint;
    QMap<std::size_t, StatePtr_t> waypointStates;

    // Initialize all states as non-waypoints
    for (const StatePtr_t& state : states) {
      if (!state) continue;
      nodeIsWaypoint[state->id()] = false;
    }

    // Find WaypointEdges and mark their internal states as waypoints
    for (const StatePtr_t& state : states) {
      if (!state) continue;
      for (const EdgePtr_t& edge : state->neighborEdges()) {
        if (!edge) continue;
        WaypointEdgePtr_t waypointEdge =
            std::dynamic_pointer_cast<WaypointEdge>(edge);
        if (waypointEdge && waypointEdge->nbWaypoints() > 0) {
          // Get waypoint states from internal edges
          for (std::size_t i = 0; i <= waypointEdge->nbWaypoints(); ++i) {
            EdgePtr_t innerEdge = waypointEdge->waypoint(i);
            if (innerEdge) {
              StatePtr_t toState = innerEdge->stateTo();
              if (toState && i < waypointEdge->nbWaypoints()) {
                nodeIsWaypoint[toState->id()] = true;
                waypointStates[toState->id()] = toState;
              }
            }
          }
        }
      }
    }

    // Helper lambda to add a state node to the scene
    auto addStateNode = [&](const StatePtr_t& state, bool isWaypoint,
                            bool isFirst) {
      QString nodeName = QString::fromStdString(state->name());
      nodeName.replace(" : ", "\n");

      QGVNode* n = scene_->addNode(nodeName);
      if (isFirst) {
        scene_->setRootNode(n);
      }

      NodeInfo ni;
      ni.id = state->id();
      ni.name = QString::fromStdString(state->name());
      ni.node = n;
      ni.isWaypoint = isWaypoint;
      ni.constraintStr = getConstraints(state);

      nodeInfos_[n] = ni;
      nodes_[ni.id] = n;

      n->setFlag(QGraphicsItem::ItemIsMovable, true);
      n->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

      // Mark waypoint nodes with different shape
      if (isWaypoint) {
        n->setAttribute("shape", "hexagon");
      }
    };

    // Add regular states (always visible)
    bool first = true;
    for (const StatePtr_t& state : states) {
      if (!state) continue;

      // Skip waypoint states if hiding waypoints
      if (hideWaypoints && nodeIsWaypoint.value(state->id(), false)) {
        continue;
      }

      addStateNode(state, nodeIsWaypoint.value(state->id(), false), first);
      first = false;
    }

    // Add waypoint states if showing waypoints
    if (!hideWaypoints) {
      for (auto it = waypointStates.begin(); it != waypointStates.end(); ++it) {
        if (!nodes_.contains(it.key())) {
          addStateNode(it.value(), true, first);
          first = false;
        }
      }
    }

    // Helper lambda to add an edge to the scene
    auto addEdgeToScene = [&](const EdgePtr_t& edge, long weight,
                              std::size_t nbWaypoints) {
      if (!edge) return;
      if (edges_[edge->id()] != nullptr) return;
      StatePtr_t fromState = edge->stateFrom();
      StatePtr_t toState = edge->stateTo();
      StatePtr_t containingState = edge->state();

      if (!fromState || !toState) return;

      if (!nodes_.contains(fromState->id()) || !nodes_.contains(toState->id()))
        return;

      QGVNode* fromNode = nodes_[fromState->id()];
      QGVNode* toNode = nodes_[toState->id()];

      QGVEdge* e = scene_->addEdge(fromNode, toNode, "");

      EdgeInfo ei;
      ei.id = edge->id();
      ei.name = QString::fromStdString(edge->name());
      ei.edge = e;
      ei.isShort = edge->isShort();
      ei.weight = weight;
      ei.nbWaypoints = nbWaypoints;

      if (containingState) {
        ei.containingStateName =
            QString::fromStdString(containingState->name());
      }

      // Build constraint string
      if (nbWaypoints > 0 && hideWaypoints) {
        ei.constraintStr =
            tr("<p><h4>Waypoint transition</h4>"
               "This transition has %1 waypoints.<br/>"
               "To see the constraints of the transitions inside,<br/>"
               "re-draw the graph after enabling \"Show waypoints\"</p>")
                .arg(nbWaypoints);
      } else {
        ei.constraintStr = getConstraints(edge);
      }

      // Update edge style
      updateEdgeStyle(e, weight);

      // For transitions inside waypoint edges, adjust layout
      if (weight < 0) {
        e->setAttribute("weight", "3");
        if (fromState->id() >= toState->id()) {
          e->setAttribute("constraint", "false");
        }
      }

      edgeInfos_[e] = ei;
      edges_[ei.id] = e;
    };

    // Add edges from each state
    for (const StatePtr_t& state : states) {
      if (!state) continue;

      // Get all outgoing edges from this state
      Edges_t neighborEdges = state->neighborEdges();

      for (const EdgePtr_t& edge : neighborEdges) {
        if (!edge) continue;

        // Get weight from the source state
        long weight = static_cast<long>(state->getWeight(edge));

        // Check if this is a waypoint edge
        WaypointEdgePtr_t waypointEdge =
            std::dynamic_pointer_cast<WaypointEdge>(edge);
        std::size_t nbWaypoints =
            waypointEdge ? waypointEdge->nbWaypoints() : 0;

        // Determine edge visibility based on waypoint settings
        bool hasWaypoints = nbWaypoints > 0;
        bool edgeVisible =
            (!hideWaypoints && !hasWaypoints) || (hideWaypoints && weight >= 0);

        if (edgeVisible) {
          addEdgeToScene(edge, weight, nbWaypoints);
        } else if (!hideWaypoints && hasWaypoints) {
          for (std::size_t i = 0; i <= nbWaypoints; ++i) {
            EdgePtr_t innerEdge = waypointEdge->waypoint(i);
            if (innerEdge) {
              // Inner edges have negative weight (from the containing state)
              long innerWeight = -1;
              addEdgeToScene(innerEdge, innerWeight, 0);
            }
          }
        }
      }
    }

    qDebug() << "Added" << nodes_.size() << "nodes and" << edges_.size()
             << "edges";

  } catch (const std::exception& e) {
    qDebug() << "Error filling scene:" << e.what();
  }
}

void HppNativeGraphWidget::updateEdgeStyle(QGVEdge* edge, long weight) {
  if (!edge) return;

  if (weight <= 0) {
    edge->setAttribute("style", "dashed");
    edge->setAttribute("penwidth", "1");
  } else {
    edge->setAttribute("style", "filled");
    edge->setAttribute("penwidth", QString::number(1 + (weight - 1) / 5));
  }
}

void HppNativeGraphWidget::nodeContextMenu(QGVNode* node) {
  if (!nodeInfos_.contains(node)) return;

  const NodeInfo& ni = nodeInfos_[node];
  long savedId = currentId_;
  currentId_ = static_cast<long>(ni.id);

  QMenu cm("Node context menu", this);

  cm.addAction("Show constraints",
               [this, &ni]() { constraintInfo_->setText(ni.constraintStr); });

  cm.addAction("Show detailed constraints",
               [this, &ni]() { displayStateConstraints(ni.id); });

  cm.addSeparator();

  cm.addAction("Highlight this state",
               [this, &ni]() { highlightNode(static_cast<long>(ni.id)); });

  cm.addAction("Clear highlight", [this]() { highlightNode(-1); });

  // Allow external code (Python) to add custom actions
  Q_EMIT nodeContextMenuAboutToShow(ni.id, ni.name, &cm);

  cm.exec(QCursor::pos());

  currentId_ = savedId;
}

void HppNativeGraphWidget::nodeDoubleClick(QGVNode* node) {
  if (!nodeInfos_.contains(node)) return;

  const NodeInfo& ni = nodeInfos_[node];

  // Display detailed constraints on double-click
  displayStateConstraints(ni.id);
}

void HppNativeGraphWidget::edgeContextMenu(QGVEdge* edge) {
  if (!edgeInfos_.contains(edge)) return;

  EdgeInfo& ei = edgeInfos_[edge];
  long savedId = currentId_;
  currentId_ = static_cast<long>(ei.id);

  QMenu cm("Edge context menu", this);

  cm.addAction("Show constraints",
               [this, &ei]() { constraintInfo_->setText(ei.constraintStr); });

  cm.addAction("Show detailed constraints",
               [this, &ei]() { displayEdgeConstraints(ei.id); });

  cm.addAction("Show target constraints",
               [this, &ei]() { displayEdgeTargetConstraints(ei.id); });

  cm.addSeparator();

  cm.addAction(QString("Weight: %1").arg(ei.weight), []() {
      // Weight display only - modification requires ProblemSolver access
    })->setEnabled(false);

  cm.addSeparator();

  cm.addAction("Highlight this edge",
               [this, &ei]() { highlightEdge(static_cast<long>(ei.id)); });

  cm.addAction("Clear highlight", [this]() { highlightEdge(-1); });

  // Allow external code (Python) to add custom actions
  Q_EMIT edgeContextMenuAboutToShow(ei.id, ei.name, &cm);

  cm.exec(QCursor::pos());

  currentId_ = savedId;
}

void HppNativeGraphWidget::edgeDoubleClick(QGVEdge* edge) {
  if (!edgeInfos_.contains(edge)) return;

  const EdgeInfo& ei = edgeInfos_[edge];

  // Display detailed constraints on double-click
  displayEdgeConstraints(ei.id);
}

QString HppNativeGraphWidget::getConstraints(
    const GraphComponentPtr_t& component) {
  if (!component) return QString();

  QString ret;
  const auto& constraints = component->numericalConstraints();

  ret.append("<p><h4>Applied constraints</h4>");
  if (!constraints.empty()) {
    ret.append("<ul>");
    for (const auto& c : constraints) {
      if (c) {
        ret.append(QString("<li>%1</li>")
                       .arg(QString::fromStdString(c->function().name())));
      }
    }
    ret.append("</ul></p>");
  } else {
    ret.append("No constraints applied</p>");
  }

  return ret;
}

QString HppNativeGraphWidget::getDetailedStateConstraints(std::size_t stateId) {
  if (!graph_) return QString();

  // Find the state by ID
  auto selector = graph_->stateSelector();
  if (!selector) return QString();

  StatePtr_t targetState;
  for (const auto& state : selector->getStates()) {
    if (state && state->id() == stateId) {
      targetState = state;
      break;
    }
  }

  if (!targetState) return QString("<p>State not found</p>");

  std::ostringstream oss;
  oss << "<h4>State: " << targetState->name() << "</h4>\n";

  // Get the config projector
  auto configProjector = targetState->configConstraint();
  if (configProjector) {
    oss << "<h5>Configuration Constraints:</h5>\n";
    oss << "<pre>" << *configProjector << "</pre>\n";
  }

  // Path constraints
  auto pathConstraints = targetState->numericalConstraintsForPath();
  if (!pathConstraints.empty()) {
    oss << "<h5>Path Constraints:</h5>\n<ul>";
    for (const auto& c : pathConstraints) {
      if (c) {
        oss << "<li>" << c->function().name() << "</li>\n";
      }
    }
    oss << "</ul>\n";
  }

  return QString::fromStdString(oss.str());
}

QString HppNativeGraphWidget::getDetailedEdgeConstraints(std::size_t edgeId) {
  if (!graph_) return QString();

  // Find the edge by ID
  auto selector = graph_->stateSelector();
  if (!selector) return QString();

  EdgePtr_t targetEdge;
  for (const auto& state : selector->getStates()) {
    for (const auto& edge : state->neighborEdges()) {
      if (edge && edge->id() == edgeId) {
        targetEdge = edge;
        break;
      }
    }
    if (targetEdge) break;
  }

  if (!targetEdge) return QString("<p>Edge not found</p>");

  std::ostringstream oss;
  oss << "<h4>Edge: " << targetEdge->name() << "</h4>\n";

  // Path constraints (for paths belonging to this edge)
  auto pathConstraint = targetEdge->pathConstraint();
  if (pathConstraint) {
    oss << "<h5>Path Constraints:</h5>\n";
    oss << "<pre>" << *pathConstraint << "</pre>\n";
  }

  // List numerical constraints from GraphComponent base class
  const auto& numConstraints = targetEdge->numericalConstraints();
  if (!numConstraints.empty()) {
    oss << "<h5>Numerical Constraints:</h5>\n<ul>";
    for (const auto& c : numConstraints) {
      if (c) {
        oss << "<li>" << c->function().name() << "</li>\n";
      }
    }
    oss << "</ul>\n";
  }

  return QString::fromStdString(oss.str());
}

QString HppNativeGraphWidget::getDetailedEdgeTargetConstraints(
    std::size_t edgeId) {
  if (!graph_) return QString();

  // Find the edge by ID
  auto selector = graph_->stateSelector();
  if (!selector) return QString();

  EdgePtr_t targetEdge;
  for (const auto& state : selector->getStates()) {
    for (const auto& edge : state->neighborEdges()) {
      if (edge && edge->id() == edgeId) {
        targetEdge = edge;
        break;
      }
    }
    if (targetEdge) break;
  }

  if (!targetEdge) return QString("<p>Edge not found</p>");

  std::ostringstream oss;
  oss << "<h4>Edge Target: " << targetEdge->name() << "</h4>\n";

  // Get the target constraint (for reaching the target state)
  auto targetConstraint = targetEdge->targetConstraint();
  if (targetConstraint) {
    oss << "<h5>Target Constraints:</h5>\n";
    oss << "<pre>" << *targetConstraint << "</pre>\n";
  }

  return QString::fromStdString(oss.str());
}

void HppNativeGraphWidget::selectionChanged() {
  QList<QGraphicsItem*> items = scene_->selectedItems();
  currentId_ = -1;

  QString type, name;
  std::size_t id = 0;
  QString end;
  QString constraints;
  QString weight;

  if (items.size() == 0) {
    // Show graph info when nothing is selected
    if (graphInfo_.id > 0) {
      type = "Graph";
      id = graphInfo_.id;
      constraints = graphInfo_.constraintStr;
    } else {
      elmtInfo_->setText("No info");
      return;
    }
  } else if (items.size() == 1) {
    QGVNode* node = dynamic_cast<QGVNode*>(items.first());
    QGVEdge* edge = dynamic_cast<QGVEdge*>(items.first());

    if (node && nodeInfos_.contains(node)) {
      type = "State";
      const NodeInfo& ni = nodeInfos_[node];
      name = ni.name;
      id = ni.id;
      currentId_ = static_cast<long>(id);
      constraints = ni.constraintStr;

      if (ni.isWaypoint) {
        end = "<p><i>This is a waypoint state</i></p>";
      }
    } else if (edge && edgeInfos_.contains(edge)) {
      type = "Edge";
      const EdgeInfo& ei = edgeInfos_[edge];
      name = ei.name;
      id = ei.id;
      currentId_ = static_cast<long>(id);
      weight = QString("<li>Weight: %1</li>").arg(ei.weight);
      constraints = ei.constraintStr;

      end = QString("<p><h4>Containing state</h4>\n%1</p>")
                .arg(ei.containingStateName);

      if (ei.isShort) {
        end.prepend("<h4>Short edge</h4>");
      }

      if (ei.nbWaypoints > 0) {
        end.prepend(QString("<p><i>This edge has %1 waypoints</i></p>")
                        .arg(ei.nbWaypoints));
      }
    } else {
      return;
    }
  }

  elmtInfo_->setText(QString("<h4>%1 %2</h4><ul>"
                             "<li>Id: %3</li>"
                             "%4"
                             "</ul>%5%6")
                         .arg(type)
                         .arg(ESCAPE(name))
                         .arg(id)
                         .arg(weight)
                         .arg(end)
                         .arg(constraints));
}

}  // namespace plot
}  // namespace hpp
