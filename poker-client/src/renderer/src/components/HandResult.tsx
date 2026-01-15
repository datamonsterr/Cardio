import React from 'react';
import CardComponent from './card/Card';
import type { Card } from '../types';

interface HandResultProps {
  serverState: {
    betting_round: string;
    winner_seat: number;
    amount_won: number;
    winner_hand_rank?: number;
    players: Array<{
      player_id: number;
      name: string;
      seat: number;
      state: string;
      money: number;
      cards: number[];
    } | null>;
    community_cards: number[];
  };
  onContinue: () => void;
  decodeCard: (encoded: number) => Card | null;
}

// Map hand rank to display name
const getHandRankName = (rank: number | undefined): string => {
  if (rank === undefined || rank < 0) return '';
  const handNames = [
    'High Card',
    'Pair',
    'Two Pair',
    'Three of a Kind',
    'Straight',
    'Flush',
    'Full House',
    'Four of a Kind',
    'Straight Flush'
  ];
  return handNames[rank] || '';
};

const HandResult: React.FC<HandResultProps> = ({ serverState, onContinue, decodeCard }) => {
  // Count players who participated in showdown (not folded, not empty, have cards)
  const showdownPlayers = serverState.players.filter(p => 
    p && 
    p.state !== 'empty' && 
    p.state !== 'folded' && 
    p.cards && 
    p.cards.length >= 2
  );

  const isShowdown = showdownPlayers.length >= 2;
  const winner = serverState.players[serverState.winner_seat];
  
  // Debug log hand rank
  console.log('[HandResult] Winner hand rank:', serverState.winner_hand_rank, 'Winner seat:', serverState.winner_seat, 'Full state:', serverState);

  // Decode community cards
  const communityCards: Card[] = [];
  serverState.community_cards.forEach((encodedCard) => {
    const card = decodeCard(encodedCard);
    if (card) {
      communityCards.push(card);
    }
  });

  return (
    <div className="hand-result-overlay">
      <div className="hand-result-container">
        <h2 className="hand-result-title">
          {isShowdown ? 'Showdown Results' : 'Hand Complete'}
        </h2>

        {isShowdown ? (
          <>
            {/* Showdown: Compare all players' hands */}
            <div className="showdown-players">
              {showdownPlayers.map((player) => {
                if (!player) return null;
                
                const playerCards: Card[] = [];
                player.cards.forEach((encodedCard) => {
                  const card = decodeCard(encodedCard);
                  if (card) playerCards.push(card);
                });

                const isPlayerWinner = player.seat === serverState.winner_seat;

                return (
                  <div 
                    key={player.seat} 
                    className={`showdown-player ${isPlayerWinner ? 'winner' : ''}`}
                  >
                    <div className="showdown-player-header">
                      <h3>{player.name} {isPlayerWinner && 'Winner'}</h3>
                      {isPlayerWinner && (
                        <>
                          <div className="winner-badge">Won: ${serverState.amount_won.toLocaleString()}</div>
                          {serverState.winner_hand_rank !== undefined && serverState.winner_hand_rank >= 0 && (
                            <div className="winner-hand-rank">
                              {getHandRankName(serverState.winner_hand_rank)}
                            </div>
                          )}
                        </>
                      )}
                    </div>
                    <div className="showdown-player-cards">
                      {playerCards.map((card, idx) => (
                        <CardComponent key={idx} cardData={card} />
                      ))}
                    </div>
                    <div className="showdown-player-info">
                      <div>Final Chips: ${player.money.toLocaleString()}</div>
                    </div>
                  </div>
                );
              })}
            </div>

            {/* Community cards */}
            {communityCards.length > 0 && (
              <div className="showdown-community">
                <h4>Community Cards</h4>
                <div className="community-cards-display">
                  {communityCards.map((card, idx) => (
                    <CardComponent key={idx} cardData={card} />
                  ))}
                </div>
              </div>
            )}
          </>
        ) : (
          <>
            {/* Only one player left (opponents folded) */}
            {winner && (
              <div className="fold-winner">
                <h3 className="winner-name">{winner.name} Wins!</h3>
                <div className="winner-amount">${serverState.amount_won.toLocaleString()}</div>
                {serverState.winner_hand_rank !== undefined && serverState.winner_hand_rank >= 0 && (
                  <div className="winner-hand-rank">
                    {getHandRankName(serverState.winner_hand_rank)}
                  </div>
                )}
                <p className="fold-message">All opponents folded</p>
              </div>
            )}

            {/* Community cards (if any were dealt) */}
            {communityCards.length > 0 && (
              <div className="fold-community">
                <h4>Community Cards</h4>
                <div className="community-cards-display">
                  {communityCards.map((card, idx) => (
                    <CardComponent key={idx} cardData={card} />
                  ))}
                </div>
              </div>
            )}
          </>
        )}

        <div className="hand-result-actions">
          <button className="continue-button" onClick={onContinue}>
            Continue
          </button>
        </div>
      </div>
    </div>
  );
};

export default HandResult;
