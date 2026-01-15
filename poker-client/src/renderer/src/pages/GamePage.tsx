import React, { useEffect, useState, useCallback, useRef, Component, ErrorInfo, ReactNode } from 'react';
import { useNavigate, useSearchParams } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import Spinner from '../components/Spinner';
import WinScreen from '../components/WinScreen';
import HandResult from '../components/HandResult';
import CardComponent from '../components/card/Card';
import Handle from '../components/slider/Handle';
import Track from '../components/slider/Track';
import { sliderStyle, railStyle } from '../components/slider/types';
import type { Player, Card } from '../types';
import potImg from '../assets/pot.svg';
import tableImg from '../assets/table-nobg-svg-01.svg';
import boyAvatar from '../assets/boy.svg';
import { Slider, Rail, Handles, Tracks } from 'react-compound-slider';
import { encode as msgpackEncode, decode as msgpackDecode } from '@msgpack/msgpack';

// Import game CSS
import '../assets/App.css';
import '../assets/Poker.css';

// Error Boundary Component
interface ErrorBoundaryState {
  hasError: boolean;
  error: Error | null;
}

class GameErrorBoundary extends Component<{ children: ReactNode }, ErrorBoundaryState> {
  constructor(props: { children: ReactNode }) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error: Error): ErrorBoundaryState {
    return { hasError: true, error };
  }

  componentDidCatch(error: Error, errorInfo: ErrorInfo): void {
    console.error('[GameErrorBoundary] Caught error:', error, errorInfo);
  }

  render(): ReactNode {
    if (this.state.hasError) {
      return (
        <div style={{ padding: '20px', color: 'white', backgroundColor: '#1a1a2e', minHeight: '100vh' }}>
          <h1>Something went wrong</h1>
          <pre style={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word' }}>
            {this.state.error?.message}
          </pre>
          <pre style={{ whiteSpace: 'pre-wrap', wordBreak: 'break-word', fontSize: '12px' }}>
            {this.state.error?.stack}
          </pre>
          <button 
            onClick={() => window.location.href = '/lobby'}
            style={{ marginTop: '20px', padding: '10px 20px' }}
          >
            Back to Lobby
          </button>
        </div>
      );
    }
    return this.props.children;
  }
}

// Protocol constants
const PACKET_JOIN_TABLE = 400;
const PACKET_ACTION_REQUEST = 450;
const PACKET_ACTION_RESULT = 451;
const PACKET_UPDATE_GAMESTATE = 600;

// Types for server game state
interface ServerPlayer {
  player_id: number;
  name: string;
  seat: number;
  state: 'empty' | 'waiting' | 'active' | 'folded' | 'all_in' | 'sitting_out';
  money: number;
  bet: number;
  total_bet: number;
  cards: [number, number];
  is_dealer: boolean;
  is_small_blind: boolean;
  is_big_blind: boolean;
  timer_deadline: number;
}

interface AvailableAction {
  type: 'fold' | 'check' | 'call' | 'bet' | 'raise' | 'all_in';
  min_amount: number;
  max_amount: number;
  increment: number;
}

interface ServerGameState {
  game_id: number;
  hand_id: number;
  seq: number;
  max_players: number;
  small_blind: number;
  big_blind: number;
  min_buy_in: number;
  max_buy_in: number;
  betting_round: 'preflop' | 'flop' | 'turn' | 'river' | 'showdown' | 'complete';
  dealer_seat: number;
  active_seat: number;
  winner_seat: number;
  amount_won: number;
  winner_hand_rank?: number;  // 0=High Card, 1=Pair, 2=Two Pair, 3=Three Kind, 4=Straight, 5=Flush, 6=Full House, 7=Four Kind, 8=Straight Flush
  players: (ServerPlayer | null)[];
  community_cards: number[];
  main_pot: number;
  side_pots: { amount: number; eligible_players: number[] }[];
  current_bet: number;
  min_raise: number;
  available_actions: AvailableAction[];
}

interface LocalGameState {
  loading: boolean;
  connected: boolean;
  winnerFound: boolean;
  winnerSeat: number;
  amountWon: number;
  serverState: ServerGameState | null;
  error: string | null;
  betAmount: number;
  actionPending: boolean;
  showHandResult: boolean;
}

const initialState: LocalGameState = {
  loading: true,
  connected: false,
  winnerFound: false,
  winnerSeat: -1,
  amountWon: 0,
  serverState: null,
  error: null,
  betAmount: 0,
  actionPending: false,
  showHandResult: false,
};

// Card decoding helpers
function decodeCard(encoded: number): Card | null {
  if (encoded < 0) return null; // Hidden card
  
  const suit = Math.floor(encoded / 13);
  const rank = (encoded % 13) + 2;
  
  const suitNames: Record<number, Card['suit']> = {
    1: 'Spade',
    2: 'Heart',
    3: 'Diamond',
    4: 'Club',
  };
  
  const rankFaces: Record<number, Card['cardFace']> = {
    2: '2', 3: '3', 4: '4', 5: '5', 6: '6', 7: '7', 8: '8', 9: '9', 10: '10',
    11: 'J', 12: 'Q', 13: 'K', 14: 'A',
  };
  
  return {
    suit: suitNames[suit] || 'Spade',
    cardFace: rankFaces[rank] || '2',
    value: rank,
  };
}

const GamePage: React.FC = () => {
  console.log('[GamePage] Component rendering...');
  
  const { user, getClient } = useAuth();
  const navigate = useNavigate();
  const [searchParams] = useSearchParams();
  const tableId = parseInt(searchParams.get('tableId') || '0', 10);
  
  console.log('[GamePage] user:', user, 'tableId:', tableId);
  
  const [gameState, setGameState] = useState<LocalGameState>(initialState);
  const clientRef = useRef<any>(null);
  const hasJoinedRef = useRef(false);
  const handResultTimeoutRef = useRef<NodeJS.Timeout | null>(null);
  const hasRequestedRefreshRef = useRef(false);
  
  // Friend invite modal state
  const [showFriendModal, setShowFriendModal] = useState(false);
  const [friends, setFriends] = useState<any[]>([]);
  const [loadingFriends, setLoadingFriends] = useState(false);

  // Get user ID for checking if it's our turn
  const userId = user?.id ? parseInt(user.id, 10) : 0;

  // Helper functions to check game state
  const isWaitingForPlayers = () => {
    if (!gameState.serverState) return false;
    const activePlayers = gameState.serverState.players.filter(p => p && p.state !== 'empty');
    // Need at least 2 players to start a game, and game shouldn't be in progress
    return activePlayers.length < 2 || (activePlayers.length === 1 && gameState.serverState.betting_round === 'complete');
  };

  const getPlayerCount = () => {
    if (!gameState.serverState) return 0;
    return gameState.serverState.players.filter(p => p && p.state !== 'empty').length;
  };

  const isGameInProgress = () => {
    if (!gameState.serverState) return false;
    const activePlayers = gameState.serverState.players.filter(p => p && p.state !== 'empty');
    return activePlayers.length >= 2 && 
           gameState.serverState.betting_round !== 'complete' && 
           gameState.serverState.hand_id > 0;
  };

  // Friend modal functions
  const openFriendModal = () => {
    setShowFriendModal(true);
    loadFriends();
  };

  const loadFriends = async () => {
    if (!clientRef.current) return;
    
    setLoadingFriends(true);
    try {
      // Send GET_FRIENDS packet (type 910)
      const payload = new Uint8Array(msgpackEncode({}));
      const totalLen = 5 + payload.length;
      const packet = new Uint8Array(totalLen);
      const view = new DataView(packet.buffer);
      
      view.setUint16(0, totalLen, false);
      view.setUint8(2, 0x01);
      view.setUint16(3, 910, false); // GET_FRIENDS packet
      
      packet.set(payload, 5);
      clientRef.current.send(packet);
      
      // For now, set empty friends list - in real implementation, 
      // this would be populated by server response
      setTimeout(() => {
        setFriends([]);
        setLoadingFriends(false);
      }, 1000);
    } catch (error) {
      console.error('Failed to load friends:', error);
      setLoadingFriends(false);
    }
  };

  const addFriend = async (friendName: string) => {
    if (!clientRef.current || !friendName) return;
    
    try {
      // Send ADD_FRIEND packet (type 920)
      const payload = new Uint8Array(msgpackEncode({ friend_name: friendName }));
      const totalLen = 5 + payload.length;
      const packet = new Uint8Array(totalLen);
      const view = new DataView(packet.buffer);
      
      view.setUint16(0, totalLen, false);
      view.setUint8(2, 0x01);
      view.setUint16(3, 920, false); // ADD_FRIEND packet
      
      packet.set(payload, 5);
      clientRef.current.send(packet);
      
      console.log('Sent friend request to:', friendName);
    } catch (error) {
      console.error('Failed to send friend request:', error);
    }
  };

  const sendFriendInvite = async (friendName: string) => {
    if (!clientRef.current || !friendName || !tableId) return;
    
    try {
      // Send INVITE_FRIEND packet (type 960)
      const payload = new Uint8Array(msgpackEncode({ 
        friend_name: friendName,
        table_id: tableId 
      }));
      const totalLen = 5 + payload.length;
      const packet = new Uint8Array(totalLen);
      const view = new DataView(packet.buffer);
      
      view.setUint16(0, totalLen, false);
      view.setUint8(2, 0x01);
      view.setUint16(3, 960, false); // INVITE_FRIEND packet
      
      packet.set(payload, 5);
      clientRef.current.send(packet);
      
      console.log('Sent table invite to:', friendName);
    } catch (error) {
      console.error('Failed to send table invite:', error);
    }
  };

  // Handle game state update from server
  const handleGameStateUpdate = useCallback((data: Uint8Array) => {
    try {
      console.log('[GamePage] handleGameStateUpdate called with data length:', data?.length);
      if (!data || data.length === 0) {
        console.error('[GamePage] Empty data received in handleGameStateUpdate');
        return;
      }
      
      const serverState = msgpackDecode(data) as ServerGameState;
      console.log('[GamePage] Decoded game state:', JSON.stringify(serverState, null, 2));
      console.log('[GamePage] Winner hand rank:', serverState.winner_hand_rank, 'Winner seat:', serverState.winner_seat);
      
      if (!serverState || typeof serverState !== 'object') {
        console.error('[GamePage] Invalid game state received:', serverState);
        return;
      }
      
      setGameState(prev => {
        // Check for winner
        const hasWinner = serverState.betting_round === 'complete' && serverState.winner_seat >= 0;
        
        // Only show hand result if the game actually happened (at least 2 players)
        const activePlayers = serverState.players.filter(p => p && p.state !== 'empty');
        const showResult = serverState.betting_round === 'complete' && activePlayers.length >= 2;
        
        // Hide result if betting round changed from complete to something else (new hand started)
        // Also check if hand_id changed, which indicates a new hand has started
        const prevHandId = prev.serverState?.hand_id;
        const currentHandId = serverState.hand_id;
        const handChanged = prevHandId !== undefined && 
                           currentHandId !== undefined &&
                           currentHandId > prevHandId;
        
        const bettingRoundChanged = prev.serverState?.betting_round === 'complete' && 
                                   serverState.betting_round !== 'complete';
        
        const hideResult = bettingRoundChanged || handChanged;
        
        // If new hand started, always hide result
        if (hideResult) {
          console.log('[GamePage] New hand detected - hiding HandResult. Prev hand_id:', prevHandId, 'Current hand_id:', currentHandId, 'Betting round:', serverState.betting_round);
          // Clear any existing timeout
          if (handResultTimeoutRef.current) {
            clearTimeout(handResultTimeoutRef.current);
            handResultTimeoutRef.current = null;
          }
          // Reset refresh flag when new hand starts
          hasRequestedRefreshRef.current = false;
        }
        
        // If hand is complete and we're showing result, set a timeout to auto-hide after 5 seconds
        // Only request fresh game state once to avoid infinite loop
        if (showResult && !hideResult && !prev.showHandResult) {
          // Clear any existing timeout
          if (handResultTimeoutRef.current) {
            clearTimeout(handResultTimeoutRef.current);
          }
          // Reset refresh flag when showing new HandResult
          hasRequestedRefreshRef.current = false;
          
          // Auto-hide after 5 seconds
          // Don't request refresh automatically - server should broadcast UPDATE_GAMESTATE when new hand starts
          handResultTimeoutRef.current = setTimeout(() => {
            console.log('[GamePage] HandResult timeout - auto-hiding (server should send UPDATE_GAMESTATE when new hand starts)');
            // Just hide HandResult - don't spam JOIN_TABLE requests
            setGameState(prev => ({ ...prev, showHandResult: false }));
            handResultTimeoutRef.current = null;
          }, 5000);
        }
        
        return {
          ...prev,
          loading: false,
          connected: true,
          serverState,
          winnerFound: hasWinner,
          winnerSeat: hasWinner ? serverState.winner_seat : -1,
          amountWon: hasWinner ? serverState.amount_won : 0,
          betAmount: prev.betAmount || serverState.big_blind || 10, // Keep bet amount or set to big blind
          actionPending: false,
          showHandResult: showResult && !hideResult,
        };
      });
    } catch (error) {
      console.error('[GamePage] Failed to decode game state:', error);
      setGameState(prev => ({
        ...prev,
        loading: false,
        error: `Failed to decode game state: ${error instanceof Error ? error.message : 'Unknown error'}`,
      }));
    }
  }, []);

  // Handle action result from server
  const handleActionResult = useCallback((data: Uint8Array) => {
    try {
      const result = msgpackDecode(data) as { result: number; reason?: string };
      console.log('[GamePage] Received action result:', result);
      
      if (result.result !== 0) {
        setGameState(prev => ({
          ...prev,
          error: result.reason || `Action failed: ${result.result}`,
          actionPending: false,
        }));
      } else {
        setGameState(prev => ({
          ...prev,
          actionPending: false,
        }));
      }
    } catch (error) {
      console.error('[GamePage] Failed to decode action result:', error);
    }
  }, []);

  // Cleanup timeout on unmount
  useEffect(() => {
    return () => {
      if (handResultTimeoutRef.current) {
        clearTimeout(handResultTimeoutRef.current);
        handResultTimeoutRef.current = null;
      }
    };
  }, []);

  useEffect(() => {
    if (!user) {
      navigate('/login');
      return;
    }

    if (!tableId) {
      navigate('/lobby');
      return;
    }

    // Prevent double join
    if (hasJoinedRef.current) {
      return;
    }

    // Set up client and join table
    const setupGame = async () => {
      try {
        const client = getClient?.();
        console.log('[GamePage] getClient returned:', client);
        
        if (!client) {
          console.error('[GamePage] No client available');
          setGameState(prev => ({
            ...prev,
            loading: false,
            error: 'No connection to server',
          }));
          return;
        }

        // Check if client is connected
        if (typeof client.isConnected === 'function' && !client.isConnected()) {
          console.error('[GamePage] Client is not connected');
          setGameState(prev => ({
            ...prev,
            loading: false,
            error: 'Not connected to server',
          }));
          return;
        }

        clientRef.current = client;
        console.log('[GamePage] Got client, setting up packet handler');

        // Store original handler - check if method exists
        const originalOnPacket = typeof client.getPacketHandler === 'function' 
          ? client.getPacketHandler() 
          : undefined;
        
        // Set up packet handler for game updates - wrap the original handler
        const gamePacketHandler = (packet: any) => {
          try {
            console.log('[GamePage] Received packet type:', packet?.header?.packet_type);
            
            // Call original handler first
            if (originalOnPacket) originalOnPacket(packet);

            // Handle game-specific packets
            const packetType = packet?.header?.packet_type;
            if (packetType === PACKET_JOIN_TABLE) {
              // JOIN_TABLE response contains game state
              console.log('[GamePage] Received JOIN_TABLE response');
              handleGameStateUpdate(packet.data);
            } else if (packetType === PACKET_UPDATE_GAMESTATE) {
              // Game state update broadcast
              console.log('[GamePage] Received UPDATE_GAMESTATE');
              handleGameStateUpdate(packet.data);
            } else if (packetType === PACKET_ACTION_RESULT) {
              // Action result
              console.log('[GamePage] Received ACTION_RESULT');
              handleActionResult(packet.data);
            }
          } catch (err) {
            console.error('[GamePage] Error handling packet:', err);
          }
        };
        
        // Set packet handler if method exists
        if (typeof client.setPacketHandler === 'function') {
          client.setPacketHandler(gamePacketHandler);
        } else {
          console.warn('[GamePage] setPacketHandler method not available on client');
        }

        // Send JOIN_TABLE request
        console.log('[GamePage] Sending JOIN_TABLE request for tableId:', tableId);
        const payload = new Uint8Array(msgpackEncode({ tableId: tableId }));
        
        // Build packet with header
        const totalLen = 5 + payload.length;
        const packet = new Uint8Array(totalLen);
        const view = new DataView(packet.buffer);
        
        view.setUint16(0, totalLen, false); // packet_len (big endian)
        view.setUint8(2, 0x01); // protocol_ver
        view.setUint16(3, PACKET_JOIN_TABLE, false); // packet_type (big endian)
        
        packet.set(payload, 5);
        
        // Send the packet
        if (typeof client.send === 'function') {
          client.send(packet);
          hasJoinedRef.current = true;
          console.log('[GamePage] JOIN_TABLE request sent');
        } else {
          console.error('[GamePage] send method not available on client');
          setGameState(prev => ({
            ...prev,
            loading: false,
            error: 'Cannot send to server',
          }));
        }
      } catch (error) {
        console.error('[GamePage] Failed to setup game:', error);
        setGameState(prev => ({
          ...prev,
          loading: false,
          error: `Failed to connect to game: ${error instanceof Error ? error.message : 'Unknown error'}`,
        }));
      }
    };

    setupGame();

    return () => {
      // Cleanup - could send LEAVE_TABLE request here
      hasJoinedRef.current = false;
    };
  }, [user, navigate, tableId, getClient, handleGameStateUpdate, handleActionResult]);

  // Send action to server
  const sendAction = useCallback(async (actionType: string, amount?: number) => {
    if (!clientRef.current || !gameState.serverState || gameState.actionPending) {
      return;
    }

    setGameState(prev => ({ ...prev, actionPending: true, error: null }));

    // Set a timeout to clear actionPending in case server doesn't respond
    const timeoutId = setTimeout(() => {
      setGameState(prev => {
        if (prev.actionPending) {
          console.warn('[GamePage] Action timeout - clearing pending state');
          return { ...prev, actionPending: false, error: 'Action timed out' };
        }
        return prev;
      });
    }, 5000);

    try {
      const payload = {
        game_id: gameState.serverState.game_id,
        action: {
          type: actionType,
          ...(amount !== undefined && amount > 0 ? { amount } : {}),
        },
        client_seq: Date.now() & 0xffffffff,
      };

      console.log('[GamePage] Sending action:', payload);
      const encodedPayload = new Uint8Array(msgpackEncode(payload));
      
      // Build packet with header
      const totalLen = 5 + encodedPayload.length;
      const packet = new Uint8Array(totalLen);
      const view = new DataView(packet.buffer);
      
      view.setUint16(0, totalLen, false); // packet_len
      view.setUint8(2, 0x01); // protocol_ver
      view.setUint16(3, PACKET_ACTION_REQUEST, false); // PACKET_ACTION_REQUEST
      
      packet.set(encodedPayload, 5);
      
      clientRef.current.send(packet);
    } catch (error) {
      clearTimeout(timeoutId);
      console.error('Failed to send action:', error);
      setGameState(prev => ({
        ...prev,
        actionPending: false,
        error: 'Failed to send action',
      }));
    }
  }, [gameState.serverState, gameState.actionPending]);

  // Action handlers
  const handleFold = useCallback(() => sendAction('fold'), [sendAction]);
  const handleCheck = useCallback(() => sendAction('check'), [sendAction]);
  const handleCall = useCallback(() => sendAction('call'), [sendAction]);
  const handleBet = useCallback(() => sendAction('bet', gameState.betAmount), [sendAction, gameState.betAmount]);
  const handleRaise = useCallback(() => sendAction('raise', gameState.betAmount), [sendAction, gameState.betAmount]);
  const handleAllIn = useCallback(() => sendAction('all_in'), [sendAction]);

  // Check if it's my turn
  const isMyTurn = useCallback(() => {
    if (!gameState.serverState || !gameState.serverState.players || !userId) return false;
    const activeSeat = gameState.serverState.active_seat;
    if (activeSeat < 0 || activeSeat >= gameState.serverState.players.length) return false;
    const activePlayer = gameState.serverState.players[activeSeat];
    return activePlayer?.player_id === userId;
  }, [gameState.serverState, userId]);

  // Get my player
  const getMyPlayer = useCallback((): ServerPlayer | null => {
    if (!gameState.serverState || !gameState.serverState.players || !userId) return null;
    return gameState.serverState.players.find(p => p?.player_id === userId) || null;
  }, [gameState.serverState, userId]);

  // Convert server player to UI player
  const serverPlayerToUIPlayer = useCallback((serverPlayer: ServerPlayer | null, _index: number): Player | null => {
    if (!serverPlayer || serverPlayer.state === 'empty') return null;
    
    const isMe = serverPlayer.player_id === userId;
    const cards: Card[] = [];
    
    // Show cards if they're mine, during showdown, or if all remaining players are all-in
    const isShowdown = gameState.serverState?.betting_round === 'showdown' || 
                       gameState.serverState?.betting_round === 'complete';
    
    // Check if all remaining players (not folded, not empty) are all-in
    const activePlayers = gameState.serverState?.players?.filter(p => 
      p && p.state !== 'empty' && p.state !== 'folded'
    ) || [];
    const allInPlayers = activePlayers.filter(p => p && p.state === 'all_in');
    const allPlayersAllIn = activePlayers.length >= 2 && allInPlayers.length === activePlayers.length;
    
    const showCards = isMe || isShowdown || allPlayersAllIn;
    if (showCards && Array.isArray(serverPlayer.cards)) {
      for (const cardValue of serverPlayer.cards) {
        const card = decodeCard(cardValue);
        if (card) cards.push(card);
      }
    }

    return {
      name: serverPlayer.name,
      chips: serverPlayer.money,
      position: serverPlayer.seat,
      cards: showCards ? cards : undefined,
      bet: serverPlayer.bet,
      folded: serverPlayer.state === 'folded',
      allIn: serverPlayer.state === 'all_in',
      isActive: gameState.serverState?.active_seat === serverPlayer.seat,
      avatarURL: boyAvatar,
    };
  }, [userId, gameState.serverState]);

  // Get available actions
  const getAvailableActions = useCallback((): AvailableAction[] => {
    if (!isMyTurn() || !gameState.serverState) return [];
    return gameState.serverState.available_actions || [];
  }, [isMyTurn, gameState.serverState]);

  // Handle bet slider change
  const handleBetChange = useCallback((values: readonly number[]) => {
    setGameState(prev => ({ ...prev, betAmount: values[0] }));
  }, []);

  // Get bet range for slider
  const getBetRange = useCallback(() => {
    const actions = getAvailableActions();
    const betAction = actions.find(a => a.type === 'bet');
    const raiseAction = actions.find(a => a.type === 'raise');
    const action = betAction || raiseAction;
    
    if (action) {
      return {
        min: action.min_amount,
        max: action.max_amount,
        step: action.increment || gameState.serverState?.big_blind || 10,
      };
    }
    
    const myPlayer = getMyPlayer();
    return {
      min: gameState.serverState?.big_blind || 10,
      max: myPlayer?.money || 1000,
      step: gameState.serverState?.big_blind || 10,
    };
  }, [getAvailableActions, getMyPlayer, gameState.serverState]);

  // Render community cards
  const renderCommunityCards = () => {
    if (!gameState.serverState || !Array.isArray(gameState.serverState.community_cards)) return null;
    
    return gameState.serverState.community_cards.map((cardValue, index) => {
      const card = decodeCard(cardValue);
      if (!card) return null;
      return <CardComponent key={index} cardData={card} />;
    });
  };

  // Render players around the table
  const renderPlayers = () => {
    if (!gameState.serverState || !Array.isArray(gameState.serverState.players)) return null;
    
    return gameState.serverState.players.map((serverPlayer, idx) => {
      // Skip null/empty players
      if (!serverPlayer) return null;
      
      const player = serverPlayerToUIPlayer(serverPlayer, idx);
      if (!player) return null;
      
      const isActive = gameState.serverState?.active_seat === idx;
      const isDealer = serverPlayer.is_dealer;
      const isSB = serverPlayer.is_small_blind;
      const isBB = serverPlayer.is_big_blind;
      
      return (
        <div 
          key={idx}
          className={`player-spot p${idx} ${isActive ? 'active' : ''} ${player.folded ? 'folded' : ''}`}
        >
          <div className="player-info">
            {/* Dealer/Blind badges */}
            <div className="player-badges">
              {isDealer && <span className="badge dealer">D</span>}
              {isSB && <span className="badge sb">SB</span>}
              {isBB && <span className="badge bb">BB</span>}
            </div>
            
            <img 
              src={player.avatarURL} 
              alt={player.name} 
              width={60} 
              height={60} 
              style={{ borderRadius: '50%', border: isActive ? '3px solid #4CAF50' : 'none' }} 
            />
            <div style={{ fontWeight: 'bold', color: isActive ? '#4CAF50' : 'white' }}>
              {player.name}
            </div>
            <div>💰 {player.chips}</div>
            {player.bet && player.bet > 0 && (
              <div className="player-bet">Bet: {player.bet}</div>
            )}
            {player.allIn && <div className="all-in-badge">ALL IN</div>}
            
            {/* Add Friend button for other players */}
            {serverPlayer.player_id !== userId && (
              <button 
                className="add-friend-btn"
                onClick={() => addFriend(player.name)}
                title={`Add ${player.name} as friend`}
              >
                + Friend
              </button>
            )}
            
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
      );
    });
  };

  // Render the game
  const renderGame = () => {
    const { serverState, betAmount } = gameState;
    
    // Don't render if no server state
    if (!serverState) {
      return (
        <div className="poker-app--background">
          <div className="poker-table--container">
            <img
              className="poker-table--table-image"
              src={tableImg}
              alt="Poker Table"
            />
            <div className="waiting-message">Waiting for game state...</div>
          </div>
        </div>
      );
    }
    
    const pot = serverState.main_pot || 0;
    const myTurn = isMyTurn();
    const availableActions = getAvailableActions();
    const betRange = getBetRange();
    
    // Check if we can bet or raise
    const canBet = availableActions.some(a => a.type === 'bet');
    const canRaise = availableActions.some(a => a.type === 'raise');
    const canCall = availableActions.some(a => a.type === 'call');
    const canCheck = availableActions.some(a => a.type === 'check');
    const canFold = availableActions.some(a => a.type === 'fold');
    const canAllIn = availableActions.some(a => a.type === 'all_in');

    return (
      <div className="poker-app--background">
        <div className="poker-table--container">
          <img
            className="poker-table--table-image"
            src={tableImg}
            alt="Poker Table"
          />
          
          {/* Betting round indicator */}
          {serverState && serverState.betting_round && isGameInProgress() && (
            <div className="betting-round-indicator">
              <span className="round-name">{serverState.betting_round.toUpperCase()}</span>
              <span className="hand-info">Hand #{serverState.hand_id}</span>
            </div>
          )}
          
          {/* Waiting for players display */}
          {isWaitingForPlayers() && (
            <div className="waiting-for-players">
              <div className="waiting-box">
                <h3>Waiting for players...</h3>
                <p>{getPlayerCount() < 2 
                  ? `Need ${2 - getPlayerCount()} more player${2 - getPlayerCount() === 1 ? '' : 's'} to start` 
                  : 'Waiting for next hand...'}
                </p>
                <p>Current players: {getPlayerCount()}</p>
                <div className="waiting-actions">
                  <button 
                    className="invite-friends-btn"
                    onClick={openFriendModal}
                  >
                    Invite Friends
                  </button>
                </div>
              </div>
            </div>
          )}

          {/* Players around the table */}
          <div className="players-container">
            {renderPlayers()}
          </div>

          {/* Community cards */}
          <div className="community-card-container">
            {renderCommunityCards()}
          </div>
          
          {/* Pot display */}
          <div className="pot-container">
            <img src={potImg} alt="Pot" style={{ height: 55, width: 55 }} />
            <h4>{pot}</h4>
          </div>
        </div>

        {/* Error display */}
        {gameState.error && (
          <div className="error-toast">
            {gameState.error}
            <button onClick={() => setGameState(prev => ({ ...prev, error: null }))}>×</button>
          </div>
        )}

        {/* Winner announcement */}
        {gameState.winnerFound && serverState && Array.isArray(serverState.players) && (
          <div className="winner-announcement">
            <h2>
              {serverState.players[gameState.winnerSeat]?.name || 'Player'} wins {gameState.amountWon}!
            </h2>
          </div>
        )}

        {/* Bet/Raise slider - vertical on right side */}
        {myTurn && !gameState.winnerFound && (canBet || canRaise) && (
          <div className="bet-slider-container">
            <label>
              {canBet ? 'Bet' : 'Raise'}
            </label>
            <Slider
              vertical
              mode={1}
              step={betRange.step}
              domain={[betRange.min, betRange.max]}
              rootStyle={sliderStyle}
              values={[betAmount]}
              onChange={handleBetChange}
            >
              <Rail>
                {({ getRailProps }) => (
                  <div style={railStyle} {...getRailProps()} />
                )}
              </Rail>
              <Handles>
                {({ handles, getHandleProps }) => (
                  <div className="slider-handles">
                    {handles.map(handle => (
                      <Handle
                        key={handle.id}
                        handle={handle}
                        getHandleProps={getHandleProps}
                      />
                    ))}
                  </div>
                )}
              </Handles>
              <Tracks right={false}>
                {({ tracks, getTrackProps }) => (
                  <div className="slider-tracks">
                    {tracks.map(({ id, source, target }) => (
                      <Track
                        key={id}
                        source={source}
                        target={target}
                        getTrackProps={getTrackProps}
                      />
                    ))}
                  </div>
                )}
              </Tracks>
            </Slider>
          </div>
        )}

        {/* Action bar - only show when it's my turn */}
        {myTurn && !gameState.winnerFound && (
          <div className="game-action-bar">
            {/* Action buttons */}
            <div className="action-buttons">
              {canFold && (
                <button 
                  className="action-button fold-button" 
                  onClick={handleFold}
                  disabled={gameState.actionPending}
                >
                  Fold
                </button>
              )}
              {canCheck && (
                <button 
                  className="action-button check-button" 
                  onClick={handleCheck}
                  disabled={gameState.actionPending}
                >
                  Check
                </button>
              )}
              {canCall && (
                <button 
                  className="action-button call-button" 
                  onClick={handleCall}
                  disabled={gameState.actionPending}
                >
                  Call {serverState?.current_bet || 0}
                </button>
              )}
              {canBet && (
                <button 
                  className="action-button bet-button" 
                  onClick={handleBet}
                  disabled={gameState.actionPending}
                >
                  Bet {betAmount}
                </button>
              )}
              {canRaise && (
                <button 
                  className="action-button raise-button" 
                  onClick={handleRaise}
                  disabled={gameState.actionPending}
                >
                  Raise to {betAmount}
                </button>
              )}
              {canAllIn && (
                <button 
                  className="action-button all-in-button" 
                  onClick={handleAllIn}
                  disabled={gameState.actionPending}
                >
                  All In
                </button>
              )}
            </div>

            {gameState.actionPending && (
              <div className="action-pending">Processing...</div>
            )}
          </div>
        )}

        {/* Waiting message when not my turn */}
        {!myTurn && !gameState.winnerFound && !isWaitingForPlayers() && serverState && Array.isArray(serverState.players) && serverState.active_seat >= 0 && serverState.active_seat < serverState.players.length && (
          <div className="waiting-message">
            Waiting for {serverState.players[serverState.active_seat]?.name || 'opponent'}...
          </div>
        )}

        {/* Leave table button */}
        <button 
          className="leave-table-button"
          onClick={() => navigate('/lobby')}
        >
          Leave Table
        </button>
      </div>
    );
  };

  if (!user) {
    console.log('[GamePage] No user, returning loading state');
    return (
      <div className="App">
        <div className="poker-table--wrapper">
          <Spinner />
        </div>
      </div>
    );
  }

  console.log('[GamePage] Rendering main component, gameState:', {
    loading: gameState.loading,
    connected: gameState.connected,
    error: gameState.error,
    hasServerState: !!gameState.serverState,
  });

  return (
    <div className="App">
      <div className="poker-table--wrapper">
        {gameState.loading ? (
          <Spinner />
        ) : gameState.error && !gameState.serverState ? (
          <div className="error-container">
            <h2>Connection Error</h2>
            <p>{gameState.error}</p>
            <button onClick={() => navigate('/lobby')}>Back to Lobby</button>
          </div>
        ) : gameState.showHandResult && gameState.serverState && gameState.serverState.betting_round === 'complete' && !isWaitingForPlayers() ? (
          <HandResult
            serverState={gameState.serverState}
            decodeCard={decodeCard}
            onContinue={() => {
              // Hide HandResult immediately when user clicks Continue
              if (handResultTimeoutRef.current) {
                clearTimeout(handResultTimeoutRef.current);
                handResultTimeoutRef.current = null;
              }
              setGameState(prev => ({ ...prev, showHandResult: false }));
              
              // Request fresh game state once to check if new hand started
              // But only if we haven't requested already
              if (!hasRequestedRefreshRef.current) {
                hasRequestedRefreshRef.current = true;
                const client = clientRef.current;
                if (client && typeof client.send === 'function' && tableId > 0) {
                  setTimeout(() => {
                    try {
                      const payload = new Uint8Array(msgpackEncode({ tableId: tableId }));
                      const totalLen = 5 + payload.length;
                      const packet = new Uint8Array(totalLen);
                      const view = new DataView(packet.buffer);
                      view.setUint16(0, totalLen, false);
                      view.setUint8(2, 0x01);
                      view.setUint16(3, PACKET_JOIN_TABLE, false);
                      packet.set(payload, 5);
                      client.send(packet);
                      console.log('[GamePage] Requested fresh game state after Continue');
                    } catch (err) {
                      console.error('[GamePage] Failed to request fresh game state:', err);
                    }
                  }, 500);
                }
              }
            }}
          />
        ) : gameState.winnerFound && gameState.amountWon > 0 && gameState.serverState && gameState.serverState.max_players <= 1 ? (
          <WinScreen 
            isWinner={gameState.winnerSeat >= 0 && gameState.serverState?.players?.[gameState.winnerSeat]?.player_id === userId}
            winnerName={gameState.serverState?.players?.[gameState.winnerSeat]?.name || 'Player'}
            amountWon={gameState.amountWon}
          />
        ) : (
          renderGame()
        )}
        
        {/* Friend Invite Modal */}
        {showFriendModal && (
          <div className="modal-overlay" onClick={() => setShowFriendModal(false)}>
            <div className="friend-modal" onClick={(e) => e.stopPropagation()}>
              <div className="modal-header">
                <h3>Invite Friends to Table</h3>
                <button className="close-btn" onClick={() => setShowFriendModal(false)}>×</button>
              </div>
              <div className="modal-content">
                {loadingFriends ? (
                  <div className="loading-friends">
                    <div className="spinner"></div>
                    <p>Loading friends...</p>
                  </div>
                ) : friends.length === 0 ? (
                  <div className="no-friends">
                    <p>No friends found. Add some friends first!</p>
                  </div>
                ) : (
                  <div className="friends-list">
                    {friends.map(friend => (
                      <div key={friend.id} className={`friend-item ${friend.online ? 'online' : 'offline'}`}>
                        <div className="friend-info">
                          <span className="friend-name">{friend.name}</span>
                          <span className={`friend-status ${friend.online ? 'online' : 'offline'}`}>
                            {friend.online ? '● Online' : '○ Offline'}
                          </span>
                        </div>
                        <button 
                          className="invite-btn"
                          onClick={() => sendFriendInvite(friend.name)}
                          disabled={!friend.online}
                        >
                          Invite
                        </button>
                      </div>
                    ))}
                  </div>
                )}
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};

// Wrap the GamePage with an error boundary for better error reporting
const GamePageWithErrorBoundary: React.FC = () => {
  return (
    <GameErrorBoundary>
      <GamePage />
    </GameErrorBoundary>
  );
};

export default GamePageWithErrorBoundary;
