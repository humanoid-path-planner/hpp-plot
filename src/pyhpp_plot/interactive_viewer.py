"""Interactive constraint graph viewer with Python callbacks.

This module provides a wrapper around the native graph viewer that allows
Python code to add custom actions to context menus for nodes and edges.
Implements all features from the CORBA hpp-monitoring-plugin.
"""


class InteractiveGraphViewer:
    """Wrapper for HppNativeGraphWidget with Python-based context menu actions."""

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
            edge_callback=self._on_edge_context_menu,
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
            "&Generate random config", lambda: self._generate_random_config(node_name)
        )

        menu.addAction(
            "Generate from &current config",
            lambda: self._generate_from_current_config(node_name),
        )

        menu.addAction(
            "Set as &target state", lambda: self._set_target_state(node_name)
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
            "&Extend current config", lambda: self._extend_current_to_current(edge_name)
        )

        menu.addAction(
            "&Extend current config to random config",
            lambda: self._extend_current_to_random(edge_name),
        )

    def _generate_random_config(self, state_name):
        """Generate random config and project to state.

        Args:
            state_name: Name of the state
        """
        try:
            state = self.graph.getState(state_name)
            shooter = self.problem.configurationShooter()

            min_error = float("inf")
            for i in range(20):
                q_random = shooter.shoot()
                success, q_proj, error = self.graph.applyStateConstraints(
                    state, q_random
                )

                if success:
                    self.current_config = q_proj
                    self.config_callback(
                        q_proj, f"Random config in state: {state_name}"
                    )
                    return True, f"Random config generated in state '{state_name}'"

                if error < min_error:
                    min_error = error

            return False, f"Failed to apply constraints (best error: {min_error:.4f})"
        except Exception as e:
            return False, f"Exception: {e!s}"

    def _generate_from_current_config(self, state_name):
        """Project current config to state.

        Args:
            state_name: Name of the state
        """
        try:
            if self.current_config is None:
                return False, "No current configuration set (generate one first)"

            state = self.graph.getState(state_name)
            success, q_proj, error = self.graph.applyStateConstraints(
                state, self.current_config
            )

            if success:
                self.current_config = q_proj
                self.config_callback(
                    q_proj, f"Current config projected to state: {state_name}"
                )
                return True, f"Configuration successfully projected to '{state_name}'"

            return False, f"Failed to project (error: {error:.4f})"
        except Exception as e:
            return False, f"Exception: {e!s}"

    def _set_target_state(self, state_name):
        """Set state as goal for planning.

        Args:
            state_name: Name of the state
        """
        try:
            state = self.graph.getState(state_name)
            shooter = self.problem.configurationShooter()

            min_error = float("inf")
            for i in range(20):
                q_random = shooter.shoot()
                success, q_goal, error = self.graph.applyStateConstraints(
                    state, q_random
                )

                if success:
                    self.problem.addGoalConfig(q_goal)
                    return True, f"Target configuration added in state '{state_name}'"

                if error < min_error:
                    min_error = error

            return False, f"Failed to generate target (best error: {min_error:.4f})"
        except Exception as e:
            return False, f"Exception: {e!s}"

    def _extend_current_to_current(self, edge_name):
        """Generate target config along edge from current config.
        calls generateTargetConfig(edge, current, current).

        Args:
            edge_name: Name of the edge
        """
        try:
            if self.current_config is None:
                return False, "No current configuration set"

            edge = self.graph.getTransition(edge_name)
            success, q_out, error = self.graph.generateTargetConfig(
                edge, self.current_config, self.current_config
            )

            if success:
                self.current_config = q_out
                self.config_callback(q_out, f"Extended along edge: {edge_name}")
                return True, f"Successfully extended along '{edge_name}'"

            return False, f"Failed to extend along edge (error code/status: {error})"
        except Exception as e:
            return False, f"Exception: {e!s}"

    def _extend_current_to_random(self, edge_name):
        """Generate target config along edge to random config.
        calls generateTargetConfig(edge, current, random).

        Args:
            edge_name: Name of the edge
        """
        try:
            if self.current_config is None:
                return False, "No current configuration set"

            edge = self.graph.getTransition(edge_name)
            shooter = self.problem.configurationShooter()
            q_random = shooter.shoot()

            success, q_out, error = self.graph.generateTargetConfig(
                edge, self.current_config, q_random
            )

            if success:
                self.current_config = q_out
                self.config_callback(
                    q_out, f"Extended along edge to random: {edge_name}"
                )
                return (
                    True,
                    f"Successfully extended along '{edge_name}' to random target",
                )

            return False, f"Failed to extend along edge (error code/status: {error})"
        except Exception as e:
            return False, f"Exception: {e!s}"

    def _resolve_state_name(self, state_identifier):
        """Resolve a state identifier from React (id or name) to a graph state name."""
        if state_identifier is None:
            return None

        token = str(state_identifier)

        try:
            self.graph.getState(token)
            return token
        except Exception:
            pass

        try:
            states = self.graph.getStates() or []
        except Exception:
            states = []

        for state in states:
            try:
                if str(state.id()) == token:
                    return state.name()
            except Exception:
                continue

        return None

    def _resolve_edge_name(self, edge_identifier):
        """Resolve an edge identifier from React (id or name) to a graph edge name."""
        if edge_identifier is None:
            return None

        token = str(edge_identifier)

        try:
            self.graph.getTransition(token)
            return token
        except Exception:
            pass

        try:
            transitions = self.graph.getTransitions() or []
        except Exception:
            transitions = []

        for edge in transitions:
            try:
                if str(edge.id()) == token:
                    return edge.name()
            except Exception:
                continue

        return None

    def handle_web_app_message(self, message):
        """Handle websocket actions coming from the React app."""

        if not isinstance(message, dict):
            return {
                "status": "error",
                "message": "Invalid message format",
                "type": "error",
            }

        if message.get("type") != "menu_action":
            return None

        action = message.get("action")
        element_kind = str(message.get("elementKind", "")).lower()
        element_id = message.get("elementId")

        # UI-only actions stay handled in React.
        if action in {"inspect", "highlight", "remove"}:
            return {
                "type": "action_success",
                "message": f"Action '{action}' handled localy.",
            }

        result = (False, f"Unrecognized action '{action}' for kind '{element_kind}'")
        handled = False

        if element_kind == "node":
            state_name = self._resolve_state_name(element_id)
            if state_name is None:
                return {
                    "status": "error",
                    "message": f"Could not resolve state ID: '{element_id}'",
                    "type": "error",
                }

            handled = True
            if action == "generate_random_config":
                result = self._generate_random_config(state_name)
            elif action == "generate_from_current_config":
                result = self._generate_from_current_config(state_name)
            elif action == "set_target_state":
                result = self._set_target_state(state_name)
            else:
                handled = False

        elif element_kind == "edge":
            edge_name = self._resolve_edge_name(element_id)
            if edge_name is None:
                return {
                    "status": "error",
                    "message": f"Could not resolve edge ID: '{element_id}'",
                    "type": "error",
                }

            handled = True
            if action == "extend_current_to_current":
                result = self._extend_current_to_current(edge_name)
            elif action == "extend_current_to_random":
                result = self._extend_current_to_random(edge_name)
            else:
                handled = False

        if not handled:
            return {"status": "error", "message": result[1], "type": "error"}

        success, msg = result
        if success:
            return {
                "type": "action_success",
                "message": msg,
                "details": {"action": action, "elementId": element_id},
            }
        else:
            return {
                "type": "error",
                "status": "error",
                "message": msg,
                "details": {"action": action, "elementId": element_id},
            }
