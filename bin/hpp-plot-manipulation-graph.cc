// Copyright (c) 2015, Joseph Mirabel
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//
// This file is part of hpp-plot.
// hpp-plot is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-plot is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-plot. If not, see <http://www.gnu.org/licenses/>.

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <hpp/corbaserver/manipulation/client.hh>

#include "hpp/plot/hpp-manipulation-graph.hh"

int main(int argc, char** argv) {
  QApplication a(argc, argv);
  QMainWindow window;

  hpp::corbaServer::manipulation::Client client(argc, argv);
  client.connect();
  hpp::plot::HppManipulationGraphWidget w(&client, NULL);
  w.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  window.setCentralWidget(&w);
  window.show();
  return a.exec();
}
