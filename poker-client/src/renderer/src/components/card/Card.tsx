import React from 'react';
import type { Card } from '../../types';
import { renderUnicodeSuitSymbol, getSuitColor } from '../../utils/ui';

interface CardProps {
  cardData: Card;
  applyFoldedClassname?: boolean;
}

const CardComponent: React.FC<CardProps> = ({ cardData, applyFoldedClassname = false }) => {
  const { suit, cardFace, animationDelay = 0 } = cardData;
  
  return (
    <div
      key={`${suit} ${cardFace}`}
      className={`playing-card cardIn${applyFoldedClassname ? ' folded' : ''}`}
      style={{
        animationDelay: `${applyFoldedClassname ? 0 : animationDelay}ms`,
      }}
    >
      <h6
        style={{
          color: getSuitColor(suit),
        }}
      >
        {`${cardFace} ${renderUnicodeSuitSymbol(suit)}`}
      </h6>
    </div>
  );
};

export default CardComponent;
