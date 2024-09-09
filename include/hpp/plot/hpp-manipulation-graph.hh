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

#ifndef HPP_PLOT_HPP_MANIPULATION_GRAPH_HH
#define HPP_PLOT_HPP_MANIPULATION_GRAPH_HH

#include <QAction>
#include <QPushButton>
#include <hpp/corbaserver/manipulation/client.hh>
#include <hpp/plot/graph-widget.hh>

namespace hpp {
namespace corbaServer {
namespace manipulation {
class Client;
}
}  // namespace corbaServer
namespace plot {
class HppManipulationGraphWidget;

class GraphAction : public QAction {
  Q_OBJECT

 public:
  GraphAction(HppManipulationGraphWidget* parent);

 signals:
  void activated(hpp::ID id);

 private slots:
  void transferSignal();

 private:
  HppManipulationGraphWidget* gw_;
};

class HppManipulationGraphWidget : public GraphWidget {
  Q_OBJECT

 public:
  HppManipulationGraphWidget(corbaServer::manipulation::Client* hpp_,
                             QWidget* parent);

  ~HppManipulationGraphWidget();

  void addNodeContextMenuAction(GraphAction* action);
  void addEdgeContextMenuAction(GraphAction* action);

  void client(corbaServer::manipulation::Client* hpp);

  bool selectionID(hpp::ID& id);
  void showEdge(const hpp::ID& edgeId);
  const std::string& graphName() const { return graphName_; }

 protected:
  void fillScene();

 public slots:
  void updateStatistics();
  void showNodeOfConfiguration(const hpp::floatSeq& cfg);
  void displayNodeConstraint(hpp::ID id);
  void displayEdgeConstraint(hpp::ID id);
  void displayEdgeTargetConstraint(hpp::ID id);

 protected slots:
  virtual void nodeContextMenu(QGVNode* node);
  virtual void nodeDoubleClick(QGVNode* node);
  virtual void edgeContextMenu(QGVEdge* edge);
  virtual void edgeDoubleClick(QGVEdge* edge);

  void selectionChanged();

 private slots:
  void startStopUpdateStats(bool start);

 private:
  corbaServer::manipulation::Client* manip_;

  struct GraphInfo {
    ::hpp::ID id;
    QString constraintStr;
  } graphInfo_;
  struct NodeInfo {
    ::hpp::ID id;
    QString constraintStr;
    QGVNode* node;

    ::hpp::ConfigProjStat configStat, pathStat;
    ::CORBA::Long freq;
    ::hpp::intSeq_var freqPerCC;
    NodeInfo();
  };
  struct EdgeInfo {
    ::hpp::ID id;
    QString name, containingNodeName;
    ::CORBA::Long weight;
    QString constraintStr;
    QString shortStr;
    QGVEdge* edge;

    ::hpp::ConfigProjStat configStat, pathStat;
    ::hpp::Names_t_var errors;
    ::hpp::intSeq_var freqs;

    EdgeInfo();
  };

  void updateWeight(EdgeInfo& ei, bool get = true);
  void updateWeight(EdgeInfo& ei, const ::CORBA::Long w);

  QString getConstraints(hpp::ID id);

  std::string graphName_;
  QList<GraphAction*> nodeContextMenuActions_;
  QList<GraphAction*> edgeContextMenuActions_;
  QMap<QGVNode*, NodeInfo> nodeInfos_;
  QMap<QGVEdge*, EdgeInfo> edgeInfos_;
  QMap<hpp::ID, QGVNode*> nodes_;
  QMap<hpp::ID, QGVEdge*> edges_;

  QPushButton *showWaypoints_, *statButton_;
  QTimer* updateStatsTimer_;

  hpp::ID currentId_, showNodeId_, showEdgeId_;
};
}  // namespace plot
}  // namespace hpp

#endif  // HPP_PLOT_HPP_MANIPULATION_GRAPH_HH
