import React, { useEffect, useState, useCallback, useRef } from 'react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { useAuth, getAuthService, subscribeToGamePackets } from '../contexts/AuthContext';
import Spinner from '../components/Spinner';
import WinScreen from '../components/WinScreen';
import CardComponent from '../components/card/Card';
import type { Player, Card, GamePhase } from '../types';
import type { GameState as ServerGameState, ActionRequest, Packet } from '../services/protocol';
import { 
  encodePacket, 
  encodeJoinTableRequest, 
  encodeActionRequest,
  decodeGameState,
  decodeActionResult,
  decodeGenericResponse,
  PACKET_TYPE,
  PROTOCOL_V1
} from '../services/protocol';
import { convertServerGameState } from '../utils/gameStateConverter';
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
  const [searchParams] = useSearchParams();
  const tableId = searchParams.get('tableId');
  const [gameState, setGameState] = useState<GameState>(initialState);
  const [serverGameState, setServerGameState] = useState<ServerGameState | null>(null);
  const [error, setError] = useState<string | null>(null);
  const clientSeqRef = useRef<number>(0);

  useEffect(() => {
    if (!user) {
      navigate('/login');
      return;
    }

    if (!tableId) {
      setError('No table ID provided');
      navigate('/tables');
      return;
    }

    // Subscribe to game packets
    const unsubscribe = subscribeToGamePackets((packet: Packet) => {
      handleGamePacket(packet);
    });

    // Get AuthService instance to access TcpClient
    const initializeGame = async () => {
      try {
        setError(null);
        const authService = getAuthService();
        const client = authService.getClient();
        
        if (!client || !client.isConnected()) {
          setError('Not connected to server. Please login again.');
          navigate('/login');
          return;
        }

        // Join table
        const joinPayload = encodeJoinTableRequest({ table_id: parseInt(tableId) });
        const joinPacket = encodePacket(PROTOCOL_V1, PACKET_TYPE.JOIN_TABLE_REQUEST, joinPayload);
        client.send(joinPacket.data);

        setGameState(prev => ({ ...prev, loading: true }));
      } catch (err) {
        console.error('Failed to initialize game:', err);
        setError(err instanceof Error ? err.message : 'Failed to join table');
        setGameState(prev => ({ ...prev, loading: false }));
      }
    };

    initializeGame();

    return () => {
      unsubscribe();
    };
  }, [user, navigate, tableId]);

  const handleGamePacket = (packet: Packet): void => {
    const packetType = packet.header.packet_type;

    try {
      if (packetType === PACKET_TYPE.JOIN_TABLE_RESPONSE) {
        // Could be GenericResponse or GameState
        try {
          const gameState = decodeGameState(packet.data);
          updateGameStateFromServer(gameState);
        } catch {
          const response = decodeGenericResponse(packet.data);
          if (response.res === PACKET_TYPE.R_JOIN_TABLE_OK) {
            // Join successful, wait for game state
            console.log('Join table successful, waiting for game state...');
          } else if (response.res === PACKET_TYPE.R_JOIN_TABLE_NOT_OK) {
            // Check if user is already at table
            if (response.msg && response.msg.includes('already at table')) {
              // User is already in the table, request game state
              console.log('User already at table, requesting game state...');
              // Server should send game state, but if not, we'll wait for UPDATE_GAMESTATE
              // For now, just wait - server should send game state
            } else {
              setError(response.msg || 'Failed to join table');
              setGameState(prev => ({ ...prev, loading: false }));
            }
          } else {
            setError(response.msg || 'Failed to join table');
            setGameState(prev => ({ ...prev, loading: false }));
          }
        }
      } else if (packetType === PACKET_TYPE.UPDATE_GAMESTATE) {
        const gameState = decodeGameState(packet.data);
        updateGameStateFromServer(gameState);
      } else if (packetType === PACKET_TYPE.ACTION_RESULT) {
        const result = decodeActionResult(packet.data);
        if (result.result !== 0) {
          setError(result.reason || 'Action failed');
        }
        // Game state will be updated via UPDATE_GAMESTATE packet
      }
    } catch (err) {
      console.error('Failed to handle game packet:', err);
    }
  };

  const updateGameStateFromServer = (serverState: ServerGameState): void => {
    setServerGameState(serverState);
    
    if (!user) return;

    const converted = convertServerGameState(serverState, user.id ? parseInt(user.id) : 0);
    
    setGameState(prev => ({
      ...prev,
      loading: false,
      players: converted.players,
      activePlayerIndex: converted.activePlayerIndex,
      dealerIndex: converted.dealerIndex,
      communityCards: converted.communityCards,
      pot: converted.pot,
      highBet: converted.highBet,
      betInputValue: converted.betInputValue,
      minBet: converted.minBet,
      phase: converted.phase,
    }));
  };

  const sendAction = useCallback(async (actionType: string, amount?: number) => {
    if (!serverGameState) {
      setError('Game state not available');
      return;
    }

    const authService = getAuthService();
    const client = authService.getClient();
    if (!client || !client.isConnected()) {
      setError('Not connected to server');
      return;
    }

    try {
      const actionRequest: ActionRequest = {
        game_id: serverGameState.game_id,
        action: {
          type: actionType,
          ...(amount !== undefined && { amount })
        },
        client_seq: ++clientSeqRef.current
      };

      const payload = encodeActionRequest(actionRequest);
      const packet = encodePacket(PROTOCOL_V1, PACKET_TYPE.ACTION_REQUEST, payload);
      client.send(packet.data);
      setError(null);
    } catch (err) {
      console.error('Failed to send action:', err);
      setError(err instanceof Error ? err.message : 'Failed to send action');
    }
  }, [serverGameState]);


  const handleNextRound = useCallback(() => {
    // Server will handle next round automatically
    // This is just for UI state
    setGameState(prev => ({
      ...prev,
      phase: 'loading'
    }));
  }, []);

  const renderCommunityCards = () => {
    return gameState.communityCards.map((card, index) => (
      <CardComponent key={index} cardData={card} />
    ));
  };

  const renderGame = () => {
    const { pot, players, activePlayerIndex, phase } = gameState;
    const isMyTurn = serverGameState && 
                     serverGameState.active_seat >= 0 &&
                     serverGameState.players[serverGameState.active_seat]?.player_id === (user?.id ? parseInt(user.id) : 0);

    return (
      <div className="poker-app--background">
        {error && (
          <div style={{ 
            position: 'fixed', 
            top: '20px', 
            left: '50%', 
            transform: 'translateX(-50%)',
            background: 'rgba(220, 53, 69, 0.9)',
            color: 'white',
            padding: '1rem 2rem',
            borderRadius: '8px',
            zIndex: 1000
          }}>
            {error}
          </div>
        )}
        
        <div className="poker-table--container">
          <img
            className="poker-table--table-image"
            src={tableImg}
            alt="Poker Table"
          />
          
          <div className="players-container">
            {players.map((player, idx) => {
              if (!player) return null;
              
              // Find server player state for this player
              const serverPlayer = serverGameState?.players.find(p => p && p.name === player.name);
              const isActive = serverPlayer && serverPlayer.seat === serverGameState?.active_seat;
              
              return (
                <div 
                  key={idx}
                  className={`player-spot p${idx} ${isActive ? 'active' : ''}`}
                >
                  <div className="player-info">
                    <img src={player.avatarURL || `https://api.dicebear.com/7.x/avataaars/svg?seed=${player.name}`} 
                         alt={player.name} 
                         width={60} 
                         height={60} 
                         style={{ borderRadius: '50%' }} />
                    <div style={{ fontWeight: 'bold' }}>{player.name}</div>
                    <div>💰 {player.chips.toLocaleString()}</div>
                    {player.bet && player.bet > 0 && <div>Bet: ${player.bet.toLocaleString()}</div>}
                    {serverPlayer?.is_dealer && <div style={{ color: '#ffd700' }}>Dealer</div>}
                    {serverPlayer?.is_small_blind && <div style={{ color: '#ffc107' }}>Small Blind</div>}
                    {serverPlayer?.is_big_blind && <div style={{ color: '#ff9800' }}>Big Blind</div>}
                    
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
              );
            })}
          </div>

          <div className="community-card-container">
            {renderCommunityCards()}
          </div>
          
          <div className="pot-container">
            <img src={potImg} alt="Pot" style={{ height: 55, width: 55 }} />
            <h4>${pot.toLocaleString()}</h4>
            {serverGameState && serverGameState.side_pots.length > 0 && (
              <div style={{ fontSize: '0.8em', color: '#b8c5d6' }}>
                + {serverGameState.side_pots.length} side pot(s)
              </div>
            )}
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
            {isMyTurn && !players[activePlayerIndex]?.folded && phase !== 'showdown' && (
              <>
                {serverGameState?.available_actions && serverGameState.available_actions.length > 0 && (
                  <>
                    {serverGameState.available_actions.map((action, idx) => {
                      if (action.type === 'fold') {
                        return (
                          <button 
                            key={idx}
                            className="fold-button" 
                            onClick={() => sendAction('fold')}
                          >
                            Fold
                          </button>
                        );
                      } else if (action.type === 'check') {
                        return (
                          <button 
                            key={idx}
                            className="action-button" 
                            onClick={() => sendAction('check')}
                          >
                            Check
                          </button>
                        );
                      } else if (action.type === 'call') {
                        return (
                          <button 
                            key={idx}
                            className="action-button" 
                            onClick={() => sendAction('call')}
                          >
                            Call ${action.min_amount}
                          </button>
                        );
                      } else if (action.type === 'bet' || action.type === 'raise') {
                        return (
                          <button 
                            key={idx}
                            className="action-button" 
                            onClick={() => sendAction(action.type, action.min_amount)}
                          >
                            {action.type === 'bet' ? 'Bet' : 'Raise'} ${action.min_amount}
                          </button>
                        );
                      }
                      return null;
                    })}
                  </>
                )}
              </>
            )}
            {!isMyTurn && (
              <div style={{ color: '#b8c5d6', padding: '1rem' }}>
                Waiting for other players...
              </div>
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
