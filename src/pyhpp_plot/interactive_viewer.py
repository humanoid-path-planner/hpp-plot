"""Interactive constraint graph viewer with Python callbacks.

This module provides a wrapper around the native graph viewer that allows
Python code to add custom actions to context menus for nodes and edges.
Implements all features from the CORBA hpp-monitoring-plugin.
"""

import threading
import queue


class InteractiveGraphViewer:
    """Wrapper for HppNativeGraphWidget with Python-based context menu actions.

    """

    def __init__(self, graph, problem, config_callback=None):
        """Initialize the interactive graph viewer.

        Args:
            graph: PyWGraph from pyhpp.manipulation
            problem: PyWProblem from pyhpp.manipulation
            config_callback: Optional callable(config, label) that receives
                           generated configurations
        """
        self.graph = graph
        self.problem = problem
        self.config_callback = config_callback or (lambda config, label: None)
        self.current_config = None

    def show(self):
        """Show graph viewer (blocking - runs Qt event loop until window closes)."""
        from pyhpp_plot.graph_viewer import show_interactive_graph

        show_interactive_graph(
            self.graph,
            node_callback=self._on_node_context_menu,
            edge_callback=self._on_edge_context_menu
        )

    def _on_node_context_menu(self, node_id, node_name, menu):
        """Add custom actions to node context menu.

        Args:
            node_id: ID of the node (state) - from C++ graph
            node_name: Name of the node (state) - from C++ graph
            menu: MenuActionProxy for adding actions
        """
        menu.addSeparator()

        menu.addAction(
            "&Generate random config",
            lambda: self._generate_random_config(node_name)
        )

        menu.addAction(
            "Generate from &current config",
            lambda: self._generate_from_current_config(node_name)
        )

        menu.addAction(
            "Set as &target state",
            lambda: self._set_target_state(node_name)
        )

    def _on_edge_context_menu(self, edge_id, edge_name, menu):
        """Add custom actions to edge context menu.

        Args:
            edge_id: ID of the edge (transition) - from C++ graph
            edge_name: Name of the edge (transition) - from C++ graph
            menu: MenuActionProxy for adding actions
        """
        menu.addSeparator()

        menu.addAction(
            "&Extend current config",
            lambda: self._extend_current_to_current(edge_name)
        )

        menu.addAction(
            "&Extend current config to random config",
            lambda: self._extend_current_to_random(edge_name)
        )

    def _generate_random_config(self, state_name):
        """Generate random config and project to state.

        Args:
            state_name: Name of the state
        """
        try:
            state = self.graph.getState(state_name)
            shooter = self.problem.configurationShooter()

            min_error = float('inf')
            for i in range(20):
                q_random = shooter.shoot()
                success, q_proj, error = self.graph.applyStateConstraints(
                    state, q_random)

                if success:
                    self.current_config = q_proj
                    self.config_callback(
                        q_proj, f"Random config in state: {state_name}")
                    print(f"Generated config for state '{state_name}'")
                    return

                if error < min_error:
                    min_error = error

            print(f"Failed to generate config for state '{state_name}' "
                  f"after 20 attempts (min error: {min_error:.6f})")
        except Exception as e:
            print(f"Error generating random config: {e}")
            import traceback
            traceback.print_exc()

    def _generate_from_current_config(self, state_name):
        """Project current config to state.

        Args:
            state_name: Name of the state
        """
        try:
            if self.current_config is None:
                print("No current config available. Generate a config first.")
                return

            state = self.graph.getState(state_name)
            success, q_proj, error = self.graph.applyStateConstraints(
                state, self.current_config)

            if success:
                self.current_config = q_proj
                self.config_callback(
                    q_proj, f"Current config projected to state: {state_name}")
                print(f"Projected current config to state '{state_name}'")
            else:
                print(f"Failed to project current config to state "
                      f"'{state_name}' (error: {error:.6f})")
        except Exception as e:
            print(f"Error projecting current config: {e}")
            import traceback
            traceback.print_exc()

    def _set_target_state(self, state_name):
        """Set state as goal for planning.

        Args:
            state_name: Name of the state
        """
        try:
            state = self.graph.getState(state_name)
            shooter = self.problem.configurationShooter()

            for i in range(20):
                q_random = shooter.shoot()
                success, q_goal, _error = self.graph.applyStateConstraints(
                    state, q_random)

                if success:
                    self.problem.addGoalConfig(q_goal)
                    print(f"Added goal config in state '{state_name}'")
                    return

            print(f"Failed to generate goal config for state '{state_name}'")
        except Exception as e:
            print(f"Error setting target state: {e}")
            import traceback
            traceback.print_exc()

    def _extend_current_to_current(self, edge_name):
        """Generate target config along edge from current config.
        calls generateTargetConfig(edge, current, current).

        Args:
            edge_name: Name of the edge
        """
        try:
            if self.current_config is None:
                print("No current config available. Generate a config first.")
                return

            edge = self.graph.getTransition(edge_name)
            success, q_out, error = self.graph.generateTargetConfig(
                edge, self.current_config, self.current_config)

            if success:
                self.current_config = q_out
                self.config_callback(
                    q_out, f"Extended along edge: {edge_name}")
                print(f"Extended current config along edge '{edge_name}'")
            else:
                print(f"Failed to extend along edge '{edge_name}' "
                      f"(error: {error:.6f})")
        except Exception as e:
            print(f"Error extending current config: {e}")
            import traceback
            traceback.print_exc()

    def _extend_current_to_random(self, edge_name):
        """Generate target config along edge to random config.
        calls generateTargetConfig(edge, current, random).

        Args:
            edge_name: Name of the edge
        """
        try:
            if self.current_config is None:
                print("No current config available. Generate a config first.")
                return

            edge = self.graph.getTransition(edge_name)
            shooter = self.problem.configurationShooter()
            q_random = shooter.shoot()

            success, q_out, error = self.graph.generateTargetConfig(
                edge, self.current_config, q_random)

            if success:
                self.current_config = q_out
                self.config_callback(
                    q_out, f"Extended along edge to random: {edge_name}")
                print(f"Extended to random config along edge '{edge_name}'")
            else:
                print(f"Failed to extend to random along edge '{edge_name}' "
                      f"(error: {error:.6f})")
        except Exception as e:
            print(f"Error extending to random config: {e}")
            import traceback
            traceback.print_exc()


class GraphViewerThread(threading.Thread):
    """Runs graph viewer in separate daemon thread with Qt event loop."""

    def __init__(self, graph, problem, config_callback):
        super().__init__(daemon=True, name="GraphViewerThread")
        self.graph = graph
        self.problem = problem
        self.config_callback = config_callback

    def run(self):
        viewer = InteractiveGraphViewer(
            self.graph, self.problem, self.config_callback)
        viewer.show()


def show_interactive_graph_threaded(graph, problem, config_callback):
    """Show interactive graph viewer in a separate thread (non-blocking).

    Args:
        graph: PyWGraph from pyhpp.manipulation
        problem: PyWProblem from pyhpp.manipulation
        config_callback: Callable(config, label) called when configs are generated

    Returns:
        GraphViewerThread object (already started)
    """
    thread = GraphViewerThread(graph, problem, config_callback)
    thread.start()
    return thread
