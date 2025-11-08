import { useState, useEffect } from 'preact/hooks';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faEye } from '@fortawesome/free-solid-svg-icons/faEye';
import { faEyeSlash } from '@fortawesome/free-solid-svg-icons/faEyeSlash';
import { faSpinner } from '@fortawesome/free-solid-svg-icons/faSpinner';

export default function VisualizerUploadModal({
  isOpen,
  onClose,
  onUpload,
  isUploading = false,
  shotInfo,
}) {
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [rememberCredentials, setRememberCredentials] = useState(false);

  const handleSubmit = async e => {
    e.preventDefault();
    if (!username.trim() || !password.trim()) {
      alert('Please enter both username and password');
      return;
    }

    try {
      await onUpload(username.trim(), password, rememberCredentials);

      // Save credentials to localStorage if requested
      if (rememberCredentials) {
        localStorage.setItem('visualizer_username', username.trim());
        localStorage.setItem('visualizer_remember', 'true');
      } else {
        localStorage.removeItem('visualizer_username');
        localStorage.removeItem('visualizer_remember');
      }

      // Clear form and close modal on success
      setUsername('');
      setPassword('');
      onClose();
    } catch (error) {
      // Error handling is done in parent component
      console.error('Upload failed:', error);
    }
  };

  const handleClose = () => {
    if (!isUploading) {
      setUsername('');
      setPassword('');
      onClose();
    }
  };

  // Load saved credentials when modal opens
  useEffect(() => {
    if (isOpen) {
      const savedUsername = localStorage.getItem('visualizer_username');
      const savedRemember = localStorage.getItem('visualizer_remember') === 'true';

      if (savedRemember && savedUsername) {
        setUsername(savedUsername);
        setRememberCredentials(true);
      }
    }
  }, [isOpen]);

  if (!isOpen) return null;

  return (
    <div
      className='backdrop-blur-sm bg-black/50 fixed inset-0 z-50 flex items-center justify-center p-4'
      onClick={handleClose}
    >
      <div
        className='max-h-[90vh] w-full max-w-md overflow-y-auto rounded-lg bg-white shadow-xl dark:bg-base-100'
        onClick={e => e.stopPropagation()}
      >
        <div className='p-6'>
          <div className='mb-4 flex items-center justify-between'>
            <h3 className='text-lg font-semibold'>Upload to Visualizer.coffee</h3>
            {!isUploading && (
              <button
                onClick={handleClose}
                className='text-gray-500 hover:text-gray-700 dark:text-gray-400 dark:hover:text-gray-200'
              >
                ✕
              </button>
            )}
          </div>

          {shotInfo && (
            <div className='mb-4 rounded-md'>
              <div className='flex justify-between text-sm'>
                <strong>Shot:</strong>
                <span>{shotInfo.profile}</span>
              </div>
              <div className='flex justify-between text-sm'>
                <strong>Date:</strong>
                <span>{new Date(shotInfo.timestamp * 1000).toLocaleString()}</span>
              </div>
              <div className='flex justify-between text-sm'>
                <strong>Duration:</strong>
                <span>{(shotInfo.duration / 1000).toFixed(1)}s</span>
              </div>
              {shotInfo.volume > 0 && (
                <div className='flex justify-between text-sm'>
                  <strong>Yield:</strong>
                  <span>{shotInfo.volume}g</span>
                </div>
              )}
            </div>
          )}

          <form onSubmit={handleSubmit} className='space-y-4' name='visualizer-login' method='post'>
            <div>
              <label htmlFor='username' className='mb-1 block text-sm font-medium'>
                Visualizer.coffee Username
              </label>
              <input
                id='username'
                name='username'
                type='text'
                value={username}
                onChange={e => setUsername(e.target.value)}
                disabled={isUploading}
                className='input input-bordered w-full'
                placeholder='Enter your username'
                autoComplete='username'
                required
              />
            </div>

            <div>
              <label htmlFor='password' className='mb-1 block text-sm font-medium'>
                Password
              </label>
              <div className='relative'>
                <input
                  id='password'
                  name='password'
                  type={showPassword ? 'text' : 'password'}
                  value={password}
                  onChange={e => setPassword(e.target.value)}
                  disabled={isUploading}
                  className='input input-bordered w-full'
                  placeholder='Enter your password'
                  autoComplete='current-password'
                  required
                />
                <button
                  type='button'
                  onClick={() => setShowPassword(!showPassword)}
                  disabled={isUploading}
                  className='absolute inset-y-0 right-0 flex items-center pr-3 text-gray-400 hover:text-gray-600 disabled:opacity-50'
                >
                  <FontAwesomeIcon icon={showPassword ? faEyeSlash : faEye} />
                </button>
              </div>
            </div>

            <div className='flex items-center'>
              <input
                id='remember'
                type='checkbox'
                checked={rememberCredentials}
                onChange={e => setRememberCredentials(e.target.checked)}
                disabled={isUploading}
                className='toggle toggle-primary'
              />
              <label htmlFor='remember' className='ml-2 text-sm'>
                Remember username
              </label>
            </div>

            <div className='flex justify-end space-x-3 pt-4'>
              <button
                type='button'
                onClick={handleClose}
                disabled={isUploading}
                className='btn btn-outline'
              >
                Cancel
              </button>
              <button
                type='submit'
                disabled={isUploading || !username.trim() || !password.trim()}
                className='btn btn-primary'
              >
                {isUploading && <FontAwesomeIcon icon={faSpinner} spin />}
                <span>{isUploading ? 'Uploading...' : 'Upload Shot'}</span>
              </button>
            </div>
          </form>

          <div className='mt-4 text-xs text-base-content/60'>
            <p>
              Your credentials are only used for this upload and will be stored locally only if you
              choose to remember your username.
            </p>
          </div>
        </div>
      </div>
    </div>
  );
}
