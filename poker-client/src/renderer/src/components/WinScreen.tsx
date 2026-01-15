import React from 'react';
import { useNavigate } from 'react-router-dom';

interface WinScreenProps {
  isWinner: boolean;
  winnerName: string;
  amountWon: number;
}

const WinScreen: React.FC<WinScreenProps> = ({ isWinner, winnerName, amountWon }) => {
  const navigate = useNavigate();

  return (
    <div className="win-screen-container">
      {isWinner ? (
        <>
          <h1>🏆 Congratulations! 🏆</h1>
          <h2>You Won!</h2>
          <p>You've won ${amountWon.toLocaleString()}!</p>
          {amountWon > 0 && <p>You've cleaned out the table!</p>}
        </>
      ) : (
        <>
          <h1>😔 Game Over</h1>
          <h2>You Lost</h2>
          <p>{winnerName} won ${amountWon.toLocaleString()}!</p>
        </>
      )}
      <button 
        className="win-screen-button" 
        onClick={() => navigate('/tables')}
      >
        Return to Tables
      </button>
    </div>
  );
};

export default WinScreen;
