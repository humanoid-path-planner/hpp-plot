import React from "react";
import { createRoot } from "react-dom/client";
import App from "./App.jsx";
import { NotificationProvider } from "./contexts/NotificationContext";
import "./styles.css";

createRoot(document.getElementById("root")).render(
  <React.StrictMode>
    <NotificationProvider>
      <App />
    </NotificationProvider>
  </React.StrictMode>
);
