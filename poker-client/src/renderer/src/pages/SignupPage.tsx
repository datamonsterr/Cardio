import React, { useState, FormEvent } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../contexts/AuthContext';
import type { SignupRequestType } from '../types';

const SignupPage: React.FC = () => {
  const [formData, setFormData] = useState<SignupRequestType>({
    user: '',
    pass: '',
    email: '',
    phone: '',
    fullname: '',
    country: '',
    gender: '',
    dob: ''
  });
  const [confirmPassword, setConfirmPassword] = useState('');
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(false);
  const { signup } = useAuth();
  const navigate = useNavigate();

  const handleInputChange = (
    e: React.ChangeEvent<HTMLInputElement | HTMLSelectElement>
  ): void => {
    const { name, value } = e.target;
    setFormData((prev) => ({
      ...prev,
      [name]: value
    }));
  };

  const validateForm = (): boolean => {
    // Username validation: at least 5 characters, alphanumeric and underscores only
    if (formData.user.length < 5) {
      setError('Username must be at least 5 characters');
      return false;
    }
    if (!/^[a-zA-Z0-9_]+$/.test(formData.user)) {
      setError('Username can only contain letters, numbers, and underscores');
      return false;
    }

    // Password validation: at least 10 characters, both numeric and alphabetic
    if (formData.pass.length < 10) {
      setError('Password must be at least 10 characters');
      return false;
    }
    if (!/[a-zA-Z]/.test(formData.pass) || !/[0-9]/.test(formData.pass)) {
      setError('Password must contain both letters and numbers');
      return false;
    }

    // Confirm password match
    if (formData.pass !== confirmPassword) {
      setError('Passwords do not match');
      return false;
    }

    // Email validation
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(formData.email)) {
      setError('Please enter a valid email address');
      return false;
    }

    // Phone validation (basic)
    if (formData.phone.length < 10) {
      setError('Please enter a valid phone number');
      return false;
    }

    // Required fields
    if (!formData.fullname.trim()) {
      setError('Full name is required');
      return false;
    }
    if (!formData.country.trim()) {
      setError('Country is required');
      return false;
    }
    if (!formData.gender) {
      setError('Gender is required');
      return false;
    }
    if (!formData.dob) {
      setError('Date of birth is required');
      return false;
    }

    return true;
  };

  const handleSubmit = async (e: FormEvent<HTMLFormElement>): Promise<void> => {
    e.preventDefault();
    setError('');

    if (!validateForm()) {
      return;
    }

    if (!signup) {
      setError('Signup service not available');
      return;
    }

    setLoading(true);

    try {
      await signup(formData);
      // Signup successful, redirect to login
      navigate('/login?registered=true');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Signup failed. Please try again.');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="login-container">
      <div className="login-card" style={{ maxWidth: '600px' }}>
        <div className="login-header">
          <h1>🃏 Create Account</h1>
          <p>Join the poker action!</p>
        </div>

        <form onSubmit={handleSubmit} className="login-form">
          <div className="form-row" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1rem' }}>
            <div className="form-group">
              <label htmlFor="user">Username *</label>
              <input
                id="user"
                name="user"
                type="text"
                placeholder="At least 5 characters"
                value={formData.user}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>

            <div className="form-group">
              <label htmlFor="fullname">Full Name *</label>
              <input
                id="fullname"
                name="fullname"
                type="text"
                placeholder="Your full name"
                value={formData.fullname}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>
          </div>

          <div className="form-row" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1rem' }}>
            <div className="form-group">
              <label htmlFor="email">Email *</label>
              <input
                id="email"
                name="email"
                type="email"
                placeholder="your@email.com"
                value={formData.email}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>

            <div className="form-group">
              <label htmlFor="phone">Phone *</label>
              <input
                id="phone"
                name="phone"
                type="tel"
                placeholder="1234567890"
                value={formData.phone}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>
          </div>

          <div className="form-row" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '1rem' }}>
            <div className="form-group">
              <label htmlFor="pass">Password *</label>
              <input
                id="pass"
                name="pass"
                type="password"
                placeholder="Min 10 chars, letters + numbers"
                value={formData.pass}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>

            <div className="form-group">
              <label htmlFor="confirmPassword">Confirm Password *</label>
              <input
                id="confirmPassword"
                name="confirmPassword"
                type="password"
                placeholder="Re-enter password"
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                className="form-input"
                required
                disabled={loading}
              />
            </div>
          </div>

          <div className="form-row" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: '1rem' }}>
            <div className="form-group">
              <label htmlFor="country">Country *</label>
              <input
                id="country"
                name="country"
                type="text"
                placeholder="USA"
                value={formData.country}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>

            <div className="form-group">
              <label htmlFor="gender">Gender *</label>
              <select
                id="gender"
                name="gender"
                value={formData.gender}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              >
                <option value="">Select...</option>
                <option value="Male">Male</option>
                <option value="Female">Female</option>
                <option value="Other">Other</option>
              </select>
            </div>

            <div className="form-group">
              <label htmlFor="dob">Date of Birth *</label>
              <input
                id="dob"
                name="dob"
                type="date"
                value={formData.dob}
                onChange={handleInputChange}
                className="form-input"
                required
                disabled={loading}
              />
            </div>
          </div>

          {error && <div className="error-message">{error}</div>}

          <button type="submit" className="login-button" disabled={loading}>
            {loading ? 'Creating Account...' : 'Sign Up'}
          </button>

          <div className="login-footer" style={{ marginTop: '1rem' }}>
            <p>
              Already have an account?{' '}
              <a
                href="/login"
                onClick={(e) => {
                  e.preventDefault();
                  navigate('/login');
                }}
                style={{ color: '#4CAF50', textDecoration: 'underline', cursor: 'pointer' }}
              >
                Login here
              </a>
            </p>
          </div>
        </form>
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

export default SignupPage;
