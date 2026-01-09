import React, { useState, FormEvent } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';

const LoginPage: React.FC = () => {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [connectionStatus, setConnectionStatus] = useState('');
  const { login, loading } = useAuth();
  const navigate = useNavigate();

  const handleSubmit = async (e: FormEvent<HTMLFormElement>): Promise<void> => {
    e.preventDefault();
    setError('');
    setConnectionStatus('');

    if (!username || !password) {
      setError('Please enter both username and password');
      return;
    }

    setIsLoading(true);
    setConnectionStatus('Connecting to server...');

    try {
      setConnectionStatus('Authenticating...');
      const success = await login(username, password);
      if (success) {
        setConnectionStatus('Login successful!');
        setTimeout(() => navigate('/home'), 500);
      } else {
        setError('Invalid credentials');
        setConnectionStatus('');
      }
    } catch (err) {
      const errorMsg = err instanceof Error ? err.message : 'Login failed. Please try again.';
      setError(errorMsg);
      setConnectionStatus('');
    } finally {
      setIsLoading(false);
    }
  };

  if (loading) {
    return (
      <div className="login-loading">
        <div className="spinner"></div>
      </div>
    );
  }

  return (
    <div className="login-container">
      <div className="login-card">
        <div className="login-header">
          <h1>🃏 Poker Online</h1>
          <p>Welcome back! Please login to continue</p>
        </div>

        <form onSubmit={handleSubmit} className="login-form">
          <div className="form-group">
            <label htmlFor="username">Username</label>
            <input
              id="username"
              type="text"
              placeholder="Enter your username"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              className="form-input"
            />
          </div>

          <div className="form-group">
            <label htmlFor="password">Password</label>
            <input
              id="password"
              type="password"
              placeholder="Enter your password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              className="form-input"
            />
          </div>

          {error && <div className="error-message">{error}</div>}
          
          {connectionStatus && (
            <div 
              className="success-message" 
              style={{
                padding: '0.75rem',
                marginBottom: '1rem',
                backgroundColor: '#4CAF50',
                color: 'white',
                borderRadius: '4px',
                textAlign: 'center'
              }}
            >
              {connectionStatus}
            </div>
          )}

          <button type="submit" className="login-button" disabled={isLoading}>
            {isLoading ? 'Logging in...' : 'Login'}
          </button>
        </form>

        <div className="login-footer">
          <p>
            Don't have an account?{' '}
            <a
              href="/signup"
              onClick={(e) => {
                e.preventDefault();
                navigate('/signup');
              }}
              style={{ color: '#4CAF50', textDecoration: 'underline', cursor: 'pointer' }}
            >
              Sign up here
            </a>
          </p>
        </div>
      </div>

      <div className="login-background">
        <div className="floating-card card-1">♠</div>
        <div className="floating-card card-2">♥</div>
        <div className="floating-card card-3">♦</div>
        <div className="floating-card card-4">♣</div>
      </div>
    </div>
  );
};

export default LoginPage;
