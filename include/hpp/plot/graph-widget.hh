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

#ifndef HPP_PLOT_GRAPHWIDGET_HH
#define HPP_PLOT_GRAPHWIDGET_HH

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QComboBox>
#include <QTextEdit>

#include <QGVScene.h>

namespace hpp {
  namespace plot {
    class GraphView : public QGraphicsView
    {
    public:
      GraphView (QWidget* parent = NULL);

      // QWidget interface
    protected:
      void wheelEvent(QWheelEvent *);
    };

    class GraphWidget : public QWidget
    {
      Q_OBJECT

    public:
      GraphWidget (QString name = QString (), QWidget *parent = NULL);

      ~GraphWidget ();

    public slots:
      void updateGraph ();
      void updateEdges ();
      void saveDotFile ();

    protected slots:
      virtual void nodeContextMenu(QGVNode* node);
      virtual void nodeDoubleClick(QGVNode* node);
      virtual void edgeContextMenu(QGVEdge* edge);
      virtual void edgeDoubleClick(QGVEdge* edge);

    protected:
      virtual void fillScene ();
      QGVScene* scene_;
      QWidget* buttonBox_;
      QTextEdit* elmtInfo_;
      QTextEdit* loggingInfo_;
      QTextEdit* constraintInfo_;

    private:
      GraphView* view_;
      QComboBox* algList_;
      bool layoutShouldBeFreed_;
    };
  }
}

#endif // HPP_PLOT_GRAPHWIDGET_HH
