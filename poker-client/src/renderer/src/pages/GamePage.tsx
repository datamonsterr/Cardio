import React, { useEffect, useState, useCallback } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import Spinner from '../components/Spinner';
import WinScreen from '../components/WinScreen';
import CardComponent from '../components/card/Card';
import { generateTable, checkWin, beginNextRound } from '../utils/gameLogic';
import { generateDeckOfCards, shuffle, dealCards, dealFlop } from '../utils/cards';
import { determineBlindIndices, anteUpBlinds } from '../utils/bet';
import type { Player, Card, GamePhase } from '../types';
import potImg from '../assets/pot.svg';
import tableImg from '../assets/table-nobg-svg-01.svg';

// Import game CSS
import '../assets/App.css';
import '../assets/Poker.css';

interface GameState {
  loading: boolean;
  winnerFound: boolean | null;
  players: Player[];
  activePlayerIndex: number;
  dealerIndex: number;
  deck: Card[];
  communityCards: Card[];
  pot: number;
  highBet: number;
  betInputValue: number;
  minBet: number;
  phase: GamePhase;
}

const initialState: GameState = {
  loading: true,
  winnerFound: null,
  players: [],
  activePlayerIndex: 0,
  dealerIndex: 0,
  deck: [],
  communityCards: [],
  pot: 0,
  highBet: 20,
  betInputValue: 20,
  minBet: 20,
  phase: 'loading',
};

const GamePage: React.FC = () => {
  const { user } = useAuth();
  const navigate = useNavigate();
  const [gameState, setGameState] = useState<GameState>(initialState);

  useEffect(() => {
    if (!user) {
      navigate('/login');
      return;
    }

    // Initialize game
    const initializeGame = async () => {
      const players = await generateTable();
      const dealerIndex = Math.floor(Math.random() * players.length);
      const blindIndices = determineBlindIndices(dealerIndex, players.length);
      const playersBoughtIn = anteUpBlinds(players, blindIndices, initialState.minBet);

      // Deal cards to players
      const shuffledDeck = shuffle(generateDeckOfCards());
      const { deck: deckAfterDeal, playerCards } = dealCards(shuffledDeck, playersBoughtIn.length);
      
      // Assign cards to players with animation delays
      const playersWithCards = playersBoughtIn.map((player, idx) => ({
        ...player,
        cards: playerCards[idx].map((card, cardIdx) => ({
          ...card,
          animationDelay: (idx * 100) + (cardIdx * 50),
        })),
      }));

      setGameState({
        ...initialState,
        loading: false,
        players: playersWithCards,
        activePlayerIndex: (dealerIndex + 3) % players.length,
        dealerIndex,
        deck: deckAfterDeal,
        phase: 'betting1',
      });

      // Auto-deal flop after 2 seconds
      setTimeout(() => {
        setGameState(prev => {
          const { deck: deckAfterFlop, flopCards } = dealFlop(prev.deck);
          return {
            ...prev,
            communityCards: flopCards.map((card, idx) => ({
              ...card,
              animationDelay: idx * 100,
            })),
            deck: deckAfterFlop,
            phase: 'flop',
          };
        });
      }, 2000);
    };

    initializeGame();
  }, [user, navigate]);

  const handleBetClick = useCallback(() => {
    // Simplified bet handler
    setGameState(prev => ({
      ...prev,
      pot: prev.pot + prev.betInputValue,
      activePlayerIndex: (prev.activePlayerIndex + 1) % prev.players.length,
    }));
  }, []);

  const handleFoldClick = useCallback(() => {
    setGameState(prev => {
      const newPlayers = [...prev.players];
      newPlayers[prev.activePlayerIndex] = {
        ...newPlayers[prev.activePlayerIndex],
        folded: true,
      };
      return {
        ...prev,
        players: newPlayers,
        activePlayerIndex: (prev.activePlayerIndex + 1) % prev.players.length,
      };
    });
  }, []);

  const handleNextRound = useCallback(() => {
    setGameState(prev => {
      const newState = beginNextRound(prev);
      if (checkWin(newState.players)) {
        return { ...newState, winnerFound: true };
      }
      return newState;
    });
  }, []);

  const renderCommunityCards = () => {
    return gameState.communityCards.map((card, index) => (
      <CardComponent key={index} cardData={card} />
    ));
  };

  const renderGame = () => {
    const { pot, players, activePlayerIndex, phase } = gameState;

    return (
      <div className="poker-app--background">
        <div className="poker-table--container">
          <img
            className="poker-table--table-image"
            src={tableImg}
            alt="Poker Table"
            style={{ width: '600px', height: '600px' }}
          />
          
          {/* Players would be rendered here in a circle around the table */}
          <div className="players-container">
            {players.map((player, idx) => (
              <div 
                key={idx}
                className={`player-spot p${idx} ${idx === activePlayerIndex ? 'active' : ''}`}
              >
                <div className="player-info">
                  <img src={player.avatarURL} alt={player.name} width={60} height={60} style={{ borderRadius: '50%' }} />
                  <div style={{ fontWeight: 'bold' }}>{player.name}</div>
                  <div>💰 {player.chips}</div>
                  {player.bet && player.bet > 0 && <div>Bet: {player.bet}</div>}
                  
                  {/* Player cards */}
                  {player.cards && player.cards.length > 0 && (
                    <div className="player-cards">
                      {player.cards.map((card, cardIdx) => (
                        <CardComponent 
                          key={cardIdx} 
                          cardData={card} 
                          applyFoldedClassname={player.folded}
                        />
                      ))}
                    </div>
                  )}
                </div>
              </div>
            ))}
          </div>

          <div className="community-card-container">
            {renderCommunityCards()}
          </div>
          
          <div className="pot-container">
            <img src={potImg} alt="Pot" style={{ height: 55, width: 55 }} />
            <h4>{pot}</h4>
          </div>
        </div>

        {phase === 'showdown' && (
          <div className="showdown-container">
            <h2>Round Complete!</h2>
            <button onClick={handleNextRound} className="next-round-button">
              Next Round
            </button>
          </div>
        )}

        <div className="game-action-bar">
          <div className="action-buttons">
            {!players[activePlayerIndex]?.folded && phase !== 'showdown' && (
              <>
                <button className="action-button" onClick={handleBetClick}>
                  Call/Bet
                </button>
                <button className="fold-button" onClick={handleFoldClick}>
                  Fold
                </button>
              </>
            )}
          </div>
        </div>
      </div>
    );
  };

  if (!user) return null;

  return (
    <div className="App">
      <div className="poker-table--wrapper">
        {gameState.loading ? (
          <Spinner />
        ) : gameState.winnerFound ? (
          <WinScreen />
        ) : (
          renderGame()
        )}
      </div>
    </div>
  );
};

export default GamePage;
