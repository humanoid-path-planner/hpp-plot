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

#include "hpp/plot/graph-widget.hh"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QMenu>
#include <QWheelEvent>
#include <QSplitter>
#include <QScrollBar>
#include <QGraphicsSceneDragDropEvent>
#include <qmath.h>

#include "QGVScene.h"
#include "QGVNode.h"
#include "QGVEdge.h"
#include "QGVSubGraph.h"

#include <QDebug>

namespace hpp {
  namespace plot {
    GraphView::GraphView(QWidget *parent)
      : QGraphicsView (parent)
    {
      setTransformationAnchor (AnchorUnderMouse);
      setDragMode (ScrollHandDrag);
      setBackgroundBrush (QBrush (Qt::lightGray, Qt::SolidPattern));
    }

    void GraphView::wheelEvent(QWheelEvent *event)
    {
      if (event->modifiers () == Qt::ControlModifier) {
          qreal scaleFactor = qPow(2.0, event->delta() / 240.0); //How fast we zoom
          qreal factor = transform().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
          if(0.05 < factor && factor < 10) //Zoom factor limitation
            scale(scaleFactor, scaleFactor);
        }
    }

    GraphWidget::GraphWidget(QString name, QWidget* parent)
      : QWidget (parent),
        scene_ (new QGVScene(name, 0)),
        buttonBox_ (new QWidget ()),
        elmtInfo_ (new QTextEdit ()),
        loggingInfo_ (new QTextEdit ()),
        constraintInfo_ (new QTextEdit ()),
        view_ (new GraphView (0)),
        layoutShouldBeFreed_ (false)
    {
      view_->setScene(scene_);

      elmtInfo_->setReadOnly (true);
      loggingInfo_->setReadOnly (true);
      elmtInfo_->setText ("No info");
      loggingInfo_->setText ("No info");
      elmtInfo_->setWordWrapMode(QTextOption::NoWrap);
      loggingInfo_->setWordWrapMode(QTextOption::NoWrap);

      view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
      view_->setRenderHint(QPainter::Antialiasing);
      view_->setRenderHint(QPainter::TextAntialiasing);

      QSplitter* splitter = new QSplitter ();
      splitter->setOrientation(Qt::Horizontal);
      QWidget* infoW = new QWidget ();
      QVBoxLayout* infoL = new QVBoxLayout ();
      infoL->addWidget(elmtInfo_);
      // infoL->addWidget(loggingInfo_);
      infoL->addWidget(constraintInfo_);
      infoW->setLayout(infoL);
      splitter->addWidget(infoW);
      splitter->addWidget(view_);
      QVBoxLayout* vl = new QVBoxLayout ();
      vl->addWidget(buttonBox_);
      vl->addWidget(splitter);
      splitter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
      this->setLayout(vl);

      elmtInfo_->resize(elmtInfo_->minimumSize());
      // loggingInfo_->resize(loggingInfo_->minimumSize());
      view_->resize(splitter->width() - elmtInfo_->width(), splitter->height());

      connect(scene_, SIGNAL(nodeContextMenu(QGVNode*)), SLOT(nodeContextMenu(QGVNode*)));
      connect(scene_, SIGNAL(nodeDoubleClick(QGVNode*)), SLOT(nodeDoubleClick(QGVNode*)));
      connect(scene_, SIGNAL(edgeContextMenu(QGVEdge*)), SLOT(edgeContextMenu(QGVEdge*)));
      connect(scene_, SIGNAL(edgeDoubleClick(QGVEdge*)), SLOT(edgeDoubleClick(QGVEdge*)));

      QHBoxLayout* hLayout = new QHBoxLayout (buttonBox_);
      hLayout->setMargin(0);
      algList_ = new QComboBox (this);
      algList_->addItems (QStringList () << "dot" << "neato" << "fdp" << "sfdp" << "twopi" << "circo");
             // << "patchwork" << "osage");
      QPushButton* saveas = new QPushButton (
            QIcon::fromTheme("document-save-as"), "&Save DOT file...", buttonBox_);
      QPushButton* refresh = new QPushButton (
            QIcon::fromTheme("view-refresh"), "&Refresh", buttonBox_);
      QPushButton* update = new QPushButton (
            QIcon::fromTheme("view-refresh"), "&Update edges", buttonBox_);
      buttonBox_->setLayout(hLayout);
      buttonBox_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      hLayout->setAlignment(buttonBox_, Qt::AlignRight);
      hLayout->addSpacerItem(new QSpacerItem (0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
      hLayout->addWidget(algList_);
      hLayout->addWidget(saveas);
      hLayout->addWidget(update);
      hLayout->addWidget(refresh);
      connect(saveas, SIGNAL (clicked ()), this, SLOT (saveDotFile()));
      connect(refresh, SIGNAL (clicked ()), this, SLOT (updateGraph()));
      connect(update, SIGNAL (clicked ()), this, SLOT (updateEdges()));

      connect(scene_, SIGNAL (nodeMouseRelease(QGVNode*)), this, SLOT (updateEdges()));
    }

    GraphWidget::~GraphWidget()
    {
      delete scene_;
    }

    void GraphWidget::updateGraph()
    {
      //Layout scene
      if (layoutShouldBeFreed_) scene_->freeLayout ();
      scene_->clear();
      QRectF rect = view_->sceneRect();
      rect.setWidth(0);
      rect.setHeight(0);
      view_->setSceneRect(rect);
      fillScene();
      scene_->applyLayout(algList_->currentText ());
      layoutShouldBeFreed_ = true;

      scene_->setNodePositionAttribute();
      scene_->setGraphAttribute("splines","spline");
      scene_->setGraphAttribute("overlap","false");
      scene_->setNodeAttribute("pin", "true");


      // scene_->render("canon", "debug.dot");

      //Fit in view
      // view_->fitInView(scene_->sceneRect(), Qt::KeepAspectRatio);
    }

    void GraphWidget::updateEdges()
    {
      //Layout scene
      if (layoutShouldBeFreed_) scene_->freeLayout ();
      scene_->applyLayout("nop2");
      layoutShouldBeFreed_ = true;
    }

    void GraphWidget::saveDotFile()
    {
      QString filename = QFileDialog::getSaveFileName(this, "Save DOT file",
          "./graph.dot", tr("DOT files (*.dot)"));
      if (!filename.isNull()) {
        scene_->writeGraph (filename);
      }
    }

    void GraphWidget::nodeContextMenu(QGVNode *node)
    {
      Q_UNUSED (node)
    }

    void GraphWidget::nodeDoubleClick(QGVNode *node)
    {
      Q_UNUSED (node)
    }

    void hpp::plot::GraphWidget::edgeDoubleClick(QGVEdge *edge)
    {
      Q_UNUSED (edge)
    }

    void hpp::plot::GraphWidget::edgeContextMenu(QGVEdge *edge)
    {
      Q_UNUSED (edge)
    }

    void GraphWidget::fillScene()
    {
      //Configure scene attributes
      scene_->setGraphAttribute("label", "DEMO");

      scene_->setGraphAttribute("splines", "ortho");
      scene_->setGraphAttribute("rankdir", "LR");
      //scene_->setGraphAttribute("concentrate", "true"); //Error !
      scene_->setGraphAttribute("nodesep", "0.4");

      scene_->setNodeAttribute("shape", "box");
      scene_->setNodeAttribute("style", "filled");
      scene_->setNodeAttribute("fillcolor", "white");
      scene_->setNodeAttribute("height", "1.2");
      scene_->setEdgeAttribute("minlen", "3");
      //scene_->setEdgeAttribute("dir", "both");

      //Add some nodes
      QGVNode *node1 = scene_->addNode("BOX");
      node1->setIcon(QIcon::fromTheme("view-refresh").pixmap(24,24).toImage());
      QGVNode *node2 = scene_->addNode("SERVER0");
      node2->setIcon(QImage(":/icons/Gnome-Network-Transmit-64.png"));
      node2->setFlag(QGraphicsItem::ItemIsMovable, true);
      node2->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
      QGVNode *node3 = scene_->addNode("SERVER1");
      node3->setIcon(QImage(":/icons/Gnome-Network-Transmit-64.png"));
      QGVNode *node4 = scene_->addNode("USER");
      node4->setIcon(QImage(":/icons/Gnome-Stock-Person-64.png"));
      QGVNode *node5 = scene_->addNode("SWITCH");
      node5->setIcon(QImage(":/icons/Gnome-Network-Server-64.png"));

      //Add some edges
      scene_->addEdge(node1, node2, "TTL")->setAttribute("color", "red");
      scene_->addEdge(node1, node2, "SERIAL");
      scene_->addEdge(node1, node3, "RAZ")->setAttribute("color", "blue");
      scene_->addEdge(node2, node3, "SECU");

      scene_->addEdge(node2, node4, "STATUS")->setAttribute("color", "red");

      scene_->addEdge(node4, node3, "ACK")->setAttribute("color", "red");

      scene_->addEdge(node4, node2, "TBIT");
      scene_->addEdge(node4, node2, "ETH");
      scene_->addEdge(node4, node2, "RS232");

      scene_->addEdge(node4, node5, "ETH1");
      scene_->addEdge(node2, node5, "ETH2");

      QGVSubGraph *sgraph = scene_->addSubGraph("SUB1");
      sgraph->setAttribute("label", "OFFICE");

      QGVNode *snode1 = sgraph->addNode("PC0152");
      QGVNode *snode2 = sgraph->addNode("PC0153");

      scene_->addEdge(snode1, snode2, "RT7");

      scene_->addEdge(node3, snode1, "GB8");
      scene_->addEdge(node3, snode2, "TS9");


      QGVSubGraph *ssgraph = sgraph->addSubGraph("SUB2");
      ssgraph->setAttribute("label", "DESK");
      scene_->addEdge(snode1, ssgraph->addNode("PC0155"), "S10");
    }
  }
}
