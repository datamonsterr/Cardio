import React from 'react';
import type { SliderTrack } from '../../types';

interface TrackProps {
  source: SliderTrack['source'];
  target: SliderTrack['target'];
  getTrackProps: () => Record<string, unknown>;
}

const Track: React.FC<TrackProps> = ({ source, target, getTrackProps }) => {
  return (
    <div
      style={{
        position: 'absolute',
        width: 10,
        zIndex: 1,
        left: '50%',
        transform: 'translateX(-50%)',
        backgroundColor: '#546C91',
        borderRadius: 5,
        cursor: 'pointer',
        bottom: `${source.percent}%`,
        height: `${target.percent - source.percent}%`,
      }}
      {...getTrackProps()}
    />
  );
};

export default Track;
