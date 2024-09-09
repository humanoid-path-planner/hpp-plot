// BSD 2-Clause License

// Copyright (c) 2015 - 2018, hpp-plot
// Authors: Heidy Dallard, Joseph Mirabel
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

#include "hpp/plot/hpp-manipulation-graph.hh"

#include <QGVEdge.h>
#include <QGVNode.h>
#include <QtGui/qtextdocument.h>
#include <assert.h>

#include <QDebug>
#include <QDir>
#include <QInputDialog>
#include <QLayout>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTemporaryFile>
#include <QTimer>
#include <iostream>
#include <limits>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define ESCAPE(q) Qt::escape(q)
#else
#define ESCAPE(q) q.toHtmlEscaped()
#endif

namespace hpp {
namespace plot {
namespace {
void initConfigProjStat(::hpp::ConfigProjStat& p) {
  p.success = 0;
  p.error = 0;
  p.nbObs = 0;
}
}  // namespace
GraphAction::GraphAction(HppManipulationGraphWidget* parent)
    : QAction(parent), gw_(parent) {
  connect(this, SIGNAL(triggered()), SLOT(transferSignal()));
}

void GraphAction::transferSignal() {
  hpp::ID id;
  if (gw_->selectionID(id)) emit activated(id);
}

HppManipulationGraphWidget::HppManipulationGraphWidget(
    corbaServer::manipulation::Client* hpp_, QWidget* parent)
    : GraphWidget("Manipulation graph", parent),
      manip_(hpp_),
      showWaypoints_(new QPushButton(QIcon::fromTheme("view-refresh"),
                                     "&Show waypoints", buttonBox_)),
      statButton_(new QPushButton(QIcon::fromTheme("view-refresh"),
                                  "&Statistics", buttonBox_)),
      updateStatsTimer_(new QTimer(this)),
      currentId_(-1),
      showNodeId_(-1),
      showEdgeId_(-1) {
  graphInfo_.id = -1;
  statButton_->setCheckable(true);
  showWaypoints_->setCheckable(true);
  showWaypoints_->setChecked(false);
  buttonBox_->layout()->addWidget(statButton_);
  buttonBox_->layout()->addWidget(showWaypoints_);
  updateStatsTimer_->setInterval(1000);
  updateStatsTimer_->setSingleShot(false);

  connect(updateStatsTimer_, SIGNAL(timeout()), SLOT(updateStatistics()));
  connect(statButton_, SIGNAL(clicked(bool)), SLOT(startStopUpdateStats(bool)));
  connect(scene_, SIGNAL(selectionChanged()), SLOT(selectionChanged()));
}

HppManipulationGraphWidget::~HppManipulationGraphWidget() {
  qDeleteAll(nodeContextMenuActions_);
  qDeleteAll(edgeContextMenuActions_);
  delete updateStatsTimer_;
}

void HppManipulationGraphWidget::addNodeContextMenuAction(GraphAction* action) {
  nodeContextMenuActions_.append(action);
}

void HppManipulationGraphWidget::addEdgeContextMenuAction(GraphAction* action) {
  edgeContextMenuActions_.append(action);
}

void HppManipulationGraphWidget::client(
    corbaServer::manipulation::Client* hpp) {
  manip_ = hpp;
}

bool hpp::plot::HppManipulationGraphWidget::selectionID(ID& id) {
  id = currentId_;
  return currentId_ != -1;
}

void HppManipulationGraphWidget::fillScene() {
  if (manip_ == NULL) return;
  hpp::GraphComp_var graph = new hpp::GraphComp;
  hpp::GraphElements_var elmts = new hpp::GraphElements;
  try {
    manip_->graph()->getGraph(graph.out(), elmts.out());

    graphName_ = graph->name;
    scene_->setGraphAttribute("label", QString(graph->name));

    scene_->setGraphAttribute("splines", "spline");
    // scene_->setGraphAttribute("rankdir", "LR");
    scene_->setGraphAttribute("outputorder", "edgesfirst");
    scene_->setGraphAttribute("nodesep", "0.5");
    scene_->setGraphAttribute("esep", "0.8");
    scene_->setGraphAttribute("sep", "1");

    scene_->setNodeAttribute("shape", "circle");
    scene_->setNodeAttribute("style", "filled");
    scene_->setNodeAttribute("fillcolor", "white");
    // scene_->setNodeAttribute("height", "1.2");
    // scene_->setEdgeAttribute("minlen", "3");

    graphInfo_.id = graph->id;
    graphInfo_.constraintStr = getConstraints(graphInfo_.id);

    // Add the nodes
    nodes_.clear();
    edges_.clear();
    bool hideW = !showWaypoints_->isChecked();
    QMap<hpp::ID, bool> nodeIsWaypoint;
    QMap<hpp::ID, bool> edgeVisible;

    // initialize node counters.
    for (std::size_t i = 0; i < elmts->nodes.length(); ++i) {
      if (elmts->nodes[(CORBA::ULong)i].id > graph->id) {
        ::hpp::ID id = elmts->nodes[(CORBA::ULong)i].id;
        nodeIsWaypoint[id] = false;
      }
    }

    // Find what edge are visible and count the nodes accordingly.
    for (std::size_t i = 0; i < elmts->edges.length(); ++i) {
      if (elmts->edges[(CORBA::ULong)i].id > graph->id) {
        ::hpp::ID id = elmts->edges[(CORBA::ULong)i].id;
        ::CORBA::Long weight = manip_->graph()->getWeight(id);

        for (std::size_t k = 0;
             k < elmts->edges[(CORBA::ULong)i].waypoints.length(); ++k)
          nodeIsWaypoint[elmts->edges[(CORBA::ULong)i]
                             .waypoints[(CORBA::ULong)k]] = true;

        bool hasWaypoints =
            elmts->edges[(CORBA::ULong)i].waypoints.length() > 0;
        // If    show Waypoint and this is not a waypoint edge
        //    or hide Waypoint and this is not a transition inside a
        //    WaypointEdge
        edgeVisible[id] = (!hideW && !hasWaypoints) || (hideW && weight >= 0);
      }
    }

    for (std::size_t i = 0; i < elmts->nodes.length(); ++i) {
      if (elmts->nodes[(CORBA::ULong)i].id > graph->id) {
        Q_ASSERT(nodeIsWaypoint.contains(elmts->nodes[(CORBA::ULong)i].id));

        if (hideW && nodeIsWaypoint[elmts->nodes[(CORBA::ULong)i].id]) {
          qDebug() << "Ignoring node" << elmts->nodes[(CORBA::ULong)i].name;
          continue;
        }
        QString nodeName(elmts->nodes[(CORBA::ULong)i].name);
        nodeName.replace(" : ", "\n");
        QGVNode* n = scene_->addNode(nodeName);
        if (i == 0) scene_->setRootNode(n);
        NodeInfo ni;
        ni.id = elmts->nodes[(CORBA::ULong)i].id;
        ni.node = n;
        ni.constraintStr = getConstraints(ni.id);
        nodeInfos_[n] = ni;
        n->setFlag(QGraphicsItem::ItemIsMovable, true);
        n->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        nodes_[elmts->nodes[(CORBA::ULong)i].id] = n;

        if (nodeIsWaypoint[ni.id]) n->setAttribute("shape", "hexagon");
      }
    }
    for (std::size_t i = 0; i < elmts->edges.length(); ++i) {
      if (elmts->edges[(CORBA::ULong)i].id > graph->id) {
        Q_ASSERT(edgeVisible.contains(elmts->edges[(CORBA::ULong)i].id) &&
                 nodeIsWaypoint.contains(elmts->edges[(CORBA::ULong)i].start) &&
                 nodeIsWaypoint.contains(elmts->edges[(CORBA::ULong)i].end));
        if (!edgeVisible[elmts->edges[(CORBA::ULong)i].id]) {
          qDebug() << "Ignoring edge" << elmts->edges[(CORBA::ULong)i].name;
          continue;
        }
        EdgeInfo ei;
        QGVEdge* e =
            scene_->addEdge(nodes_[elmts->edges[(CORBA::ULong)i].start],
                            nodes_[elmts->edges[(CORBA::ULong)i].end], "");
        ei.name = QString::fromLocal8Bit(elmts->edges[(CORBA::ULong)i].name);
        ei.id = elmts->edges[(CORBA::ULong)i].id;
        CORBA::String_var cnname = manip_->graph()->getContainingNode(ei.id);
        ei.containingNodeName = QString::fromLocal8Bit((char*)cnname);
        ei.edge = e;
        updateWeight(ei, true);

        if (elmts->edges[(CORBA::ULong)i].waypoints.length() > 0) {
          ei.constraintStr =
              tr("<p><h4>Waypoint transition</h4>"
                 "This transition has %1 waypoints.<br/>"
                 "To see the constraints of the transition inside,<br/>"
                 "re-draw the graph after enabling \"Show waypoints\"</p>")
                  .arg(elmts->edges[(CORBA::ULong)i].waypoints.length());
          ei.shortStr = "";
        } else {
          ei.constraintStr = getConstraints(ei.id);

          if (manip_->graph()->isShort(ei.id)) ei.shortStr = "<h4>Short</h4>";
        }

        // If this is a transition inside a WaypointEdge
        if (ei.weight < 0) {
          e->setAttribute("weight", "3");
          if (elmts->edges[(CORBA::ULong)i].start >=
              elmts->edges[(CORBA::ULong)i].end)
            e->setAttribute("constraint", "false");
        }

        edgeInfos_[e] = ei;
        edges_[ei.id] = e;
      }
    }
  } catch (const hpp::Error& e) {
    qDebug() << e.msg;
  }
}

void HppManipulationGraphWidget::updateStatistics() {
  if (manip_ == NULL) {
    updateStatsTimer_->stop();
    statButton_->setChecked(false);
    return;
  }
  try {
    foreach (QGraphicsItem* elmt, scene_->items()) {
      QGVNode* node = dynamic_cast<QGVNode*>(elmt);
      if (node) {
        NodeInfo& ni = nodeInfos_[node];
        manip_->graph()->getConfigProjectorStats(ni.id, ni.configStat,
                                                 ni.pathStat);
        ni.freq = manip_->graph()->getFrequencyOfNodeInRoadmap(
            ni.id, ni.freqPerCC.out());
        float sr = (ni.configStat.nbObs > 0) ? (float)ni.configStat.success /
                                                    (float)ni.configStat.nbObs
                                              : 0.f / 0.f;
        QString colorcode =
            (ni.configStat.nbObs > 0)
                ? QColor(255, (int)(sr * 255), (int)(sr * 255)).name()
                : "white";
        const QString& fillcolor = node->getAttribute("fillcolor");
        if (!(fillcolor == colorcode)) {
          node->setAttribute("fillcolor", colorcode);
          node->updateLayout();
        }
        continue;
      }
      QGVEdge* edge = dynamic_cast<QGVEdge*>(elmt);
      if (edge) {
        EdgeInfo& ei = edgeInfos_[edge];
        manip_->graph()->getConfigProjectorStats(ei.id, ei.configStat,
                                                 ei.pathStat);
        manip_->graph()->getEdgeStat(ei.id, ei.errors.out(), ei.freqs.out());
        float sr = (ei.configStat.nbObs > 0) ? (float)ei.configStat.success /
                                                    (float)ei.configStat.nbObs
                                              : 0.f / 0.f;
        QString colorcode = (ei.configStat.nbObs > 0)
                                ? QColor(255 - (int)(sr * 255), 0, 0).name()
                                : "";
        const QString& color = edge->getAttribute("color");
        if (!(color == colorcode)) {
          edge->setAttribute("color", colorcode);
          edge->updateLayout();
        }
        continue;
      }
    }
    scene_->update();
    selectionChanged();
  } catch (const CORBA::Exception&) {
    updateStatsTimer_->stop();
    statButton_->setChecked(false);
    throw;
  }
}

void HppManipulationGraphWidget::showNodeOfConfiguration(
    const hpp::floatSeq& cfg) {
  static bool lastlog = false;
  if (manip_ == NULL) return;
  if (showNodeId_ >= 0) {
    // Do unselect
    nodes_[showNodeId_]->setAttribute("fillcolor", "white");
    nodes_[showNodeId_]->updateLayout();
  }
  try {
    manip_->graph()->getNode(cfg, showNodeId_);
    // Do select
    if (nodes_.contains(showNodeId_)) {
      nodes_[showNodeId_]->setAttribute("fillcolor", "green");
      nodes_[showNodeId_]->updateLayout();
      scene_->update();
    } else {
      qDebug() << "Node" << showNodeId_
               << "does not exist. Refer the graph may solve the issue.";
      showNodeId_ = -1;
    }
    lastlog = false;
  } catch (const hpp::Error& e) {
    if (!lastlog)
      qDebug() << "HppManipulationGraphWidget::showNodeOfConfiguration"
               << e.msg;
    lastlog = true;
  }
}

void HppManipulationGraphWidget::showEdge(const hpp::ID& edgeId) {
  if (showEdgeId_ >= 0) {
    // Do unselect
    edges_[showEdgeId_]->setAttribute("color", "");
    edges_[showEdgeId_]->updateLayout();
  }
  showEdgeId_ = edgeId;
  // Do select
  if (edges_.contains(showEdgeId_)) {
    edges_[showEdgeId_]->setAttribute("color", "green");
    edges_[showEdgeId_]->updateLayout();
    scene_->update();
  } else {
    showEdgeId_ = -1;
  }
}

void HppManipulationGraphWidget::nodeContextMenu(QGVNode* node) {
  const NodeInfo& ni = nodeInfos_[node];
  hpp::ID id = currentId_;
  currentId_ = ni.id;

  QMenu cm("Node context menu", this);
  foreach (GraphAction* action, nodeContextMenuActions_) {
    cm.addAction(action);
  }
  cm.exec(QCursor::pos());

  currentId_ = id;
}

void HppManipulationGraphWidget::nodeDoubleClick(QGVNode* node) {
  const NodeInfo& ni = nodeInfos_[node];
  displayNodeConstraint(ni.id);
}

void HppManipulationGraphWidget::displayNodeConstraint(hpp::ID id) {
  if (manip_ == NULL) return;
  CORBA::String_var str;
  manip_->graph()->displayNodeConstraints(id, str.out());
  QString nodeStr(str);
  constraintInfo_->setText(nodeStr);
}

void HppManipulationGraphWidget::displayEdgeConstraint(hpp::ID id) {
  if (manip_ == NULL) return;
  CORBA::String_var str;
  manip_->graph()->displayEdgeConstraints(id, str.out());
  QString nodeStr(str);
  constraintInfo_->setText(nodeStr);
}

void HppManipulationGraphWidget::displayEdgeTargetConstraint(hpp::ID id) {
  if (manip_ == NULL) return;
  CORBA::String_var str;
  manip_->graph()->displayEdgeTargetConstraints(id, str.out());
  QString nodeStr(str);
  constraintInfo_->setText(nodeStr);
}

void HppManipulationGraphWidget::edgeContextMenu(QGVEdge* edge) {
  const EdgeInfo& ei = edgeInfos_[edge];
  hpp::ID id = currentId_;
  currentId_ = ei.id;

  QMenu cm("Edge context menu", this);
  foreach (GraphAction* action, edgeContextMenuActions_) {
    cm.addAction(action);
  }
  cm.exec(QCursor::pos());

  currentId_ = id;
}

void HppManipulationGraphWidget::edgeDoubleClick(QGVEdge* edge) {
  EdgeInfo& ei = edgeInfos_[edge];
  bool ok;
  ::CORBA::Long w = QInputDialog::getInt(
      this, "Update edge weight", tr("Edge %1 weight").arg(ei.name), ei.weight,
      0, std::numeric_limits<int>::max(), 1, &ok);
  if (ok) {
    updateWeight(ei, w);
    edge->updateLayout();
  }
}

void HppManipulationGraphWidget::selectionChanged() {
  QList<QGraphicsItem*> items = scene_->selectedItems();
  currentId_ = -1;

  QString type, name;
  ::hpp::ID id;
  QString end;
  QString constraints;
  QString weight;

  if (items.size() == 0) {
    if (graphInfo_.id < 0) {
      elmtInfo_->setText("No info");
      return;
    }
    type = "Graph";
    id = graphInfo_.id;
    constraints = graphInfo_.constraintStr;
  } else if (items.size() == 1) {
    QGVNode* node = dynamic_cast<QGVNode*>(items.first());
    QGVEdge* edge = dynamic_cast<QGVEdge*>(items.first());
    if (node) {
      type = "Node";
      name = node->label();
      const NodeInfo& ni = nodeInfos_[node];
      id = ni.id;
      currentId_ = id;
      constraints = ni.constraintStr;
      end = QString("<p><h4>Nb node in roadmap:</h4> %1</p>").arg(ni.freq);
      end.append("<p><h4>Nb node in roadmap per connected component</h4>\n");
      for (std::size_t i = 0; i < ni.freqPerCC->length(); ++i) {
        end.append(QString(" %1,").arg(ni.freqPerCC.in()[(CORBA::ULong)i]));
      }
      end.append("</p>");
    } else if (edge) {
      type = "Edge";
      const EdgeInfo& ei = edgeInfos_[edge];
      name = ei.name;
      id = ei.id;
      currentId_ = id;
      weight = QString("<li>Weight: %1</li>").arg(ei.weight);
      end = "<p>Extension results<ul>";
      for (std::size_t i = 0;
           i < std::min(ei.errors->length(), ei.freqs->length()); ++i) {
        end.append(QString("<li>%1: %2</li>")
                       .arg(QString(ei.errors.in()[(CORBA::ULong)i]))
                       .arg(ei.freqs.in()[(CORBA::ULong)i]));
      }
      end.append("</ul></p>");
      end.append(QString("<p><h4>Containing node</h4>\n%1</p>")
                     .arg(ei.containingNodeName));
      constraints = ei.constraintStr;
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

void HppManipulationGraphWidget::startStopUpdateStats(bool start) {
  if (start)
    updateStatsTimer_->start();
  else
    updateStatsTimer_->stop();
}

HppManipulationGraphWidget::NodeInfo::NodeInfo()
    : id(-1), freq(0), freqPerCC(new ::hpp::intSeq()) {
  initConfigProjStat(configStat);
  initConfigProjStat(pathStat);
}

HppManipulationGraphWidget::EdgeInfo::EdgeInfo() : id(-1), edge(NULL) {
  initConfigProjStat(configStat);
  initConfigProjStat(pathStat);
  errors = new Names_t();
  freqs = new intSeq();
}

void HppManipulationGraphWidget::updateWeight(EdgeInfo& ei, bool get) {
  if (manip_ == NULL) return;
  if (get) ei.weight = manip_->graph()->getWeight(ei.id);
  if (ei.edge == NULL) return;
  if (ei.weight <= 0) {
    ei.edge->setAttribute("style", "dashed");
    ei.edge->setAttribute("penwidth", "1");
  } else {
    ei.edge->setAttribute("style", "filled");
    ei.edge->setAttribute("penwidth", QString::number(1 + (ei.weight - 1 / 5)));
  }
}

void HppManipulationGraphWidget::updateWeight(EdgeInfo& ei,
                                              const ::CORBA::Long w) {
  if (manip_ == NULL) return;
  manip_->graph()->setWeight(ei.id, w);
  ei.weight = w;
  updateWeight(ei, false);
}

QString HppManipulationGraphWidget::getConstraints(hpp::ID id) {
  assert(manip_ != NULL);
  QString ret;
  hpp::Names_t_var c = new hpp::Names_t;
  manip_->graph()->getNumericalConstraints(id, c);
  ret.append("<p><h4>Applied constraints</h4>");
  if (c->length() > 0) {
    ret.append("<ul>");
    for (unsigned i = 0; i < c->length(); i++) {
      ret.append(QString("<li>%1</li>").arg(c[i].in()));
    }
    ret.append("</ul></p>");
  } else
    ret.append("No constraints applied</p>");
  return ret;
}
}  // namespace plot
}  // namespace hpp
