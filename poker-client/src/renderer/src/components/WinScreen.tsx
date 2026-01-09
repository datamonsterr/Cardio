import React from 'react';
import { useNavigate } from 'react-router-dom';

const WinScreen: React.FC = () => {
  const navigate = useNavigate();

  return (
    <div className="win-screen-container">
      <h1>🏆 Congratulations! 🏆</h1>
      <h2>You Won!</h2>
      <p>You've cleaned out the table!</p>
      <button 
        className="win-screen-button" 
        onClick={() => navigate('/home')}
      >
        Return to Lobby
      </button>
    </div>
  );
};

export default WinScreen;
