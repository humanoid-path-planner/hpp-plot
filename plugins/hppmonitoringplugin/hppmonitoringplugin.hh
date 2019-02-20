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

#ifndef HPP_PLOT_HPPWIDGETSPLUGIN_HH
#define HPP_PLOT_HPPWIDGETSPLUGIN_HH

#include <gepetto/gui/plugin-interface.hh>
#include <hpp/plot/hpp-manipulation-graph.hh>

#include <hpp/corbaserver/manipulation/client.hh>
#undef __robot_hh__
#undef __problem_hh__
#include <hpp/corbaserver/client.hh>

class QDockWidget;

namespace hpp {
  namespace plot {
    class HppMonitoringPlugin : public QObject, public gepetto::gui::PluginInterface,
				public gepetto::gui::ConnectionInterface
    {
      Q_OBJECT
      Q_INTERFACES (gepetto::gui::PluginInterface gepetto::gui::ConnectionInterface)
#if (QT_VERSION >= QT_VERSION_CHECK(5,0,0))
      Q_PLUGIN_METADATA (IID "hpp-plot.hppmonitoringplugin")
#endif

      public:
        explicit HppMonitoringPlugin ();

        virtual ~HppMonitoringPlugin ();

        // PluginInterface interface
      public:
        void init();
        QString name() const;

        // CorbaInterface
      public:
        virtual void openConnection ();
        virtual void closeConnection();
        virtual bool corbaException (int jobId, const CORBA::Exception &excep) const;

        public slots:
          void projectRandomConfigOn(hpp::ID idNode);
          bool projectCurrentConfigOn(hpp::ID idNode);
          void setTargetState (hpp::ID idEdge);
          bool extendFromCurrentToCurrentConfigOn(hpp::ID idEdge);
          bool extendFromCurrentToRandomConfigOn (hpp::ID idEdge);
          void applyCurrentConfiguration();
          void appliedConfigAtParam (int pid, double param);

    signals:
        void projectionStatus (QString status);

      private:
        bool projectConfigOn(hpp::floatSeq config, hpp::ID idNode);
        bool extendConfigOn(hpp::floatSeq from, hpp::floatSeq config, hpp::ID idEdge);

        bool projectRandomConfigOn_impl(hpp::ID idNode);

        hpp::floatSeq getCurrentConfig ();
        void setCurrentConfig (const hpp::floatSeq& q);
        QObject* hppPlugin ();

        hpp::plot::HppManipulationGraphWidget* cgWidget_;
        QList <QDockWidget*> docks_;

        hpp::corbaServer::manipulation::Client* manip_;
        hpp::corbaServer::Client* basic_;
        QObject* hppPlugin_;
    };
  } // namespace plot
} // namespace hpp

#endif // HPP_PLOT_HPPWIDGETSPLUGIN_HH
