import React from 'react';

const Spinner: React.FC = () => {
  return (
    <div className="spinner-container">
      <div className="spinner-animation"></div>
      <p>Loading Game...</p>
    </div>
  );
};

export default Spinner;
