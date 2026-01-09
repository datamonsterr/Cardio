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
        height: 10,
        zIndex: 1,
        marginTop: 35,
        backgroundColor: '#546C91',
        borderRadius: 5,
        cursor: 'pointer',
        left: `${source.percent}%`,
        width: `${target.percent - source.percent}%`,
      }}
      {...getTrackProps()}
    />
  );
};

export default Track;
