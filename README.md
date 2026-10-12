# hpp-plot

[![Pipeline status](https://gitlab.laas.fr/humanoid-path-planner/hpp-plot/badges/master/pipeline.svg)](https://gitlab.laas.fr/humanoid-path-planner/hpp-plot/commits/master)
[![Coverage report](https://gitlab.laas.fr/humanoid-path-planner/hpp-plot/badges/master/coverage.svg?job=doc-coverage)](https://gepettoweb.laas.fr/doc/humanoid-path-planner/hpp-plot/master/coverage/)
[![Code style: black](https://img.shields.io/badge/code%20style-black-000000.svg)](https://github.com/psf/black)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/humanoid-path-planner/hpp-plot/master.svg)](https://results.pre-commit.ci/latest/github/humanoid-path-planner/hpp-plot)

Display of the constraint graph of manipulation problems defined with
[hpp-manipulation](https://github.com/humanoid-path-planner/hpp-manipulation).

The graph is displayed by a web application served locally and connected to
Python through a websocket. The context menu of states and transitions can
generate configurations that are sent back to the Python script, for instance
to display them in the viewer. A Qt window is also available when hpp-plot is
built with `USE_QT=ON`.

## Usage

With the viser viewer of
[hpp-gepetto-viewer](https://github.com/humanoid-path-planner/hpp-gepetto-viewer),
register the problem and the constraint graph, then click on "Show Graph Viewer"
in the view controller:

```python
v.setProblem(problem)
v.setGraph(graph)
```

The graph viewer can also be started from a Python script:

```python
from pyhpp_plot import GraphViewerThread

def on_config(config, label):
    print(label, config)

GraphViewerThread(graph, problem, on_config).start()
```

The web application is then available at http://127.0.0.1:5177.

## Installation

hpp-plot is installed with the other HPP packages, see the
[installation instructions](https://humanoid-path-planner.github.io/hpp-doc/installation/installation.html).
Building the web application requires `npm` (CMake option `USE_JS`, default `ON`).
