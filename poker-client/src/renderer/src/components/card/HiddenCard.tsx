import React from 'react';
import type { Card } from '../../types';

interface HiddenCardProps {
  cardData: Card;
  applyFoldedClassname?: boolean;
}

const HiddenCard: React.FC<HiddenCardProps> = ({ cardData, applyFoldedClassname = false }) => {
  const { suit, cardFace, animationDelay = 0 } = cardData;
  
  return (
    <div
      key={`${suit} ${cardFace}`}
      className={`playing-card cardIn robotcard${applyFoldedClassname ? ' folded' : ''}`}
      style={{
        animationDelay: `${applyFoldedClassname ? 0 : animationDelay}ms`,
      }}
    />
  );
};

export default HiddenCard;
