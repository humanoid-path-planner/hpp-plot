import { createContext, useContext, useState, useCallback } from 'react';
import NotificationCard from '../components/NotificationCard';

const NotificationContext = createContext();

export const NotificationProvider = ({ children }) => {
  const [notification, setNotification] = useState(null);

  const notify = useCallback((message, type = 'info', details = null) => {
    setNotification({ message, type, details });
  }, []);

  const closeNotification = useCallback(() => {
    setNotification(null);
  }, []);

  return (
    <NotificationContext.Provider value={{ notify }}>
      {children}
      {notification && (
        <NotificationCard
          message={notification.message}
          type={notification.type}
          details={notification.details}
          onClose={closeNotification}
        />
      )}
    </NotificationContext.Provider>
  );
};

export const useNotification = () => useContext(NotificationContext);
