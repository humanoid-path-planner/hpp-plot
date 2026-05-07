import { useEffect, useState } from "react";
import "../styles/notificationCard.css";

export default function NotificationCard({ message, type, details, onClose }) {
  const [showDetails, setShowDetails] = useState(false);

  useEffect(() => {
    if (showDetails) return;

    const timer = setTimeout(() => {
      onClose();
    }, 5000);
    return () => clearTimeout(timer);
  }, [message, onClose, showDetails]);

  if (!message) return null;

  return (
    <div className={`notification-card ${type || "info"}`}>
      <div className="notification-header">
        <span className="notification-message">{message}</span>
        <button className="notification-close" onClick={onClose} aria-label="Close" title="Close notification">
          &times;
        </button>
      </div>

      {details && (
        <div className="notification-details-toggle">
          <button onClick={() => setShowDetails((prev) => !prev)}>
            {showDetails ? "Less details" : "More details"}
          </button>
        </div>
      )}

      {showDetails && details && (
        <div className="notification-details-content">
          {typeof details === "object" ? JSON.stringify(details, null, 2) : details}
        </div>
      )}
    </div>
  );
}
