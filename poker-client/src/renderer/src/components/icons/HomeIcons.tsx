import React from 'react';

interface IconProps {
  className?: string;
  size?: number;
  color?: string;
}

export const TablesIcon: React.FC<IconProps> = ({ className, size = 64, color = '#ffd700' }) => (
  <svg
    className={className}
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
  >
    <rect x="3" y="3" width="18" height="18" rx="2" stroke={color} strokeWidth="2" fill="none" />
    <line x1="3" y1="9" x2="21" y2="9" stroke={color} strokeWidth="2" />
    <line x1="9" y1="3" x2="9" y2="21" stroke={color} strokeWidth="2" />
    <line x1="15" y1="3" x2="15" y2="21" stroke={color} strokeWidth="2" />
    <line x1="3" y1="15" x2="21" y2="15" stroke={color} strokeWidth="2" />
  </svg>
);

export const PlayNowIcon: React.FC<IconProps> = ({ className, size = 64, color = '#ff8c00' }) => (
  <svg
    className={className}
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
  >
    <circle cx="12" cy="12" r="10" stroke={color} strokeWidth="2" fill="none" />
    <path
      d="M10 8L16 12L10 16V8Z"
      fill={color}
    />
  </svg>
);

export const ScoreboardIcon: React.FC<IconProps> = ({ className, size = 64, color = '#ffd700' }) => (
  <svg
    className={className}
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
  >
    <path
      d="M12 2L15.09 8.26L22 9.27L17 14.14L18.18 21.02L12 17.77L5.82 21.02L7 14.14L2 9.27L8.91 8.26L12 2Z"
      fill={color}
      stroke={color}
      strokeWidth="1"
    />
    <circle cx="12" cy="12" r="2" fill="#1a1f2e" />
  </svg>
);

export const FriendsIcon: React.FC<IconProps> = ({ className, size = 64, color = '#ffd700' }) => (
  <svg
    className={className}
    width={size}
    height={size}
    viewBox="0 0 24 24"
    fill="none"
    xmlns="http://www.w3.org/2000/svg"
  >
    <circle cx="9" cy="7" r="4" stroke={color} strokeWidth="2" fill="none" />
    <path
      d="M1 21C1 17.134 4.134 14 8 14C11.866 14 15 17.134 15 21"
      stroke={color}
      strokeWidth="2"
      fill="none"
    />
    <circle cx="17" cy="11" r="4" stroke={color} strokeWidth="2" fill="none" />
    <path
      d="M23 21C23 17.134 19.866 14 16 14"
      stroke={color}
      strokeWidth="2"
      fill="none"
    />
  </svg>
);
