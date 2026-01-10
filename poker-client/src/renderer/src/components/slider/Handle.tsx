import React from 'react';
import type { SliderHandle } from '../../types';

interface HandleProps {
  handle: SliderHandle;
  getHandleProps: (id: string) => Record<string, unknown>;
}

const Handle: React.FC<HandleProps> = ({ handle: { id, value, percent }, getHandleProps }) => {
  return (
    <div
      style={{
        bottom: `${percent}%`,
        left: '50%',
        transform: 'translate(-50%, 50%)',
        position: 'absolute',
        zIndex: 2,
        width: 30,
        height: 30,
        border: 0,
        textAlign: 'center',
        cursor: 'pointer',
        borderRadius: '50%',
        backgroundColor: '#2C4870',
        color: '#aaa',
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'center',
      }}
      {...getHandleProps(id)}
    >
      <div
        style={{
          position: 'absolute',
          left: '40px',
          whiteSpace: 'nowrap',
          textShadow: '2px 2px 8px rgba(0,0,0,0.95)',
          fontFamily: 'Roboto',
          fontSize: 14,
          color: '#fff',
          fontWeight: 'bold',
        }}
      >
        {value}
      </div>
    </div>
  );
};

export default Handle;
