import type { Suit, GamePhase } from '../types';

export const renderPhaseStatement = (phase: GamePhase): string => {
  switch (phase) {
    case 'loading':
      return 'Finding a Table, Please Wait';
    case 'initialDeal':
      return 'Dealing out the cards';
    case 'betting1':
      return 'Betting 1';
    case 'flop':
      return 'Flop';
    case 'betting2':
      return 'Flop';
    case 'turn':
      return 'Turn';
    case 'betting3':
      return 'Turn';
    case 'river':
      return 'River';
    case 'betting4':
      return 'River';
    case 'showdown':
      return 'Show Your Cards!';
    default:
      throw new Error('Unfamiliar phase returned from renderPhaseStatement()');
  }
};

export const renderUnicodeSuitSymbol = (suit: Suit): string => {
  switch (suit) {
    case 'Heart':
      return '\u2665';
    case 'Diamond':
      return '\u2666';
    case 'Spade':
      return '\u2660';
    case 'Club':
      return '\u2663';
    default:
      throw new Error('Unfamiliar String Received in Suit Unicode Generation');
  }
};

export const renderActionButtonText = (
  highBet: number,
  betInputValue: number,
  activePlayerChips: number,
  activePlayerBet: number
): string => {
  if (highBet === 0 && betInputValue === 0) {
    return 'Check';
  } else if (highBet === betInputValue) {
    return 'Call';
  } else if (highBet === 0 && betInputValue > highBet) {
    return 'Bet';
  } else if (
    betInputValue < highBet ||
    betInputValue === activePlayerChips + activePlayerBet
  ) {
    return 'All-In!';
  } else if (betInputValue > highBet) {
    return 'Raise';
  }
  return 'Check';
};

export const renderNetPlayerEarnings = (endChips: number, startChips: number): React.JSX.Element => {
  const netChipEarnings = endChips - startChips;
  const win = netChipEarnings > 0;
  const none = netChipEarnings === 0;
  return (
    <div
      className={`showdownPlayer--earnings ${
        win ? 'positive' : none ? '' : 'negative'
      }`}
    >
      {`${win ? '+' : ''}${netChipEarnings}`}
    </div>
  );
};

export const formatChips = (chips: number): string => {
  if (chips >= 1000000) {
    return `${(chips / 1000000).toFixed(1)}M`;
  } else if (chips >= 1000) {
    return `${(chips / 1000).toFixed(1)}K`;
  }
  return chips.toString();
};

export const getSuitColor = (suit: Suit): 'red' | 'black' => {
  return suit === 'Diamond' || suit === 'Heart' ? 'red' : 'black';
};
