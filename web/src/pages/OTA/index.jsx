import { useCallback, useContext, useEffect, useRef, useState } from 'preact/hooks';
import Card from '../../components/Card.jsx';
import { Spinner } from '../../components/Spinner.jsx';
import { ApiServiceContext, machine } from '../../services/ApiService.js';
import { downloadJson } from '../../utils/download.js';
import DebugLogs from '../../components/DebugLogs.jsx';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faCheck } from '@fortawesome/free-solid-svg-icons/faCheck';

const imageUrlToBase64 = async blob => {
  return new Promise((onSuccess, onError) => {
    try {
      const reader = new FileReader();
      reader.onload = function () {
        onSuccess(this.result);
      };
      reader.readAsDataURL(blob);
    } catch (e) {
      onError(e);
    }
  });
};

export function OTA() {
  const apiService = useContext(ApiServiceContext);
  const [isLoading, setIsLoading] = useState(true);
  const [submitting, setSubmitting] = useState(false);
  const [formData, setFormData] = useState({});
  const [phase, setPhase] = useState(0);
  const [progress, setProgress] = useState(0);
  const hasRequestedOtaSettings = useRef(false);
  const rssi = machine.value.status.rssi;

  const downloadSupportData = useCallback(async () => {
    const settingsResponse = await fetch(`/api/settings`);
    const data = await settingsResponse.json();
    delete data.wifiPassword;
    delete data.haPassword;
    const coredumpBlob = await fetch(`/api/core-dump`).then(r => r.blob());
    let coredump = await imageUrlToBase64(coredumpBlob);
    coredump = coredump.substring(coredump.indexOf('base64,') + 7);
    const supportFile = {
      settings: data,
      versions: formData,
      coredump,
    };
    const ts = Date.now();
    downloadJson(supportFile, `support-${ts}.dat`);
  }, [formData]);
  useEffect(() => {
    const listenerId = apiService.on('res:ota-settings', msg => {
      setFormData(msg);
      setIsLoading(false);
      setSubmitting(false);
    });
    return () => {
      apiService.off('res:ota-settings', listenerId);
    };
  }, [apiService]);
  useEffect(() => {
    const listenerId = apiService.on('evt:ota-progress', msg => {
      setProgress(msg.progress);
      setPhase(msg.phase);
    });
    return () => {
      apiService.off('evt:ota-progress', listenerId);
    };
  }, [apiService]);

  useEffect(() => {
    const listenerId = apiService.on('evt:history-rebuild-progress', msg => {
      setRebuildProgress({
        total: msg.total || 0,
        current: msg.current || 0,
        status: msg.status || '',
      });

      if (msg.status === 'completed' || msg.status === 'error') {
        setRebuilding(false);
        setRebuilt(msg.status === 'completed');
      }
    });
    return () => {
      apiService.off('evt:history-rebuild-progress', listenerId);
    };
  }, [apiService]);
  useEffect(() => {
    // Wait for WebSocket to be connected before requesting OTA settings
    if (machine.value.connected && !hasRequestedOtaSettings.current) {
      hasRequestedOtaSettings.current = true;
      apiService.send({ tp: 'req:ota-settings' });
    }
  }, [apiService, machine.value.connected]);

  const formRef = useRef();

  const onSubmit = useCallback(
    async e => {
      e.preventDefault();
      setSubmitting(true);
      const form = formRef.current;
      const formData = new FormData(form);
      apiService.send({ tp: 'req:ota-settings', update: true, channel: formData.get('channel') });
      setSubmitting(true);
    },
    [setFormData, formRef],
  );

  const onUpdate = useCallback(
    component => {
      apiService.send({ tp: 'req:ota-start', cp: component });
    },
    [apiService],
  );

  const [rebuilding, setRebuilding] = useState(false);
  const [rebuilt, setRebuilt] = useState(false);
  const [rebuildProgress, setRebuildProgress] = useState({ total: 0, current: 0, status: '' });
  const onHistoryRebuild = useCallback(async () => {
    setRebuilt(false);
    setRebuilding(true);
    setRebuildProgress({ total: 0, current: 0, status: 'starting' });
    apiService.send({ tp: 'req:history:rebuild' });
  }, [apiService]);

  if (phase > 0) {
    return (
      <div className='flex flex-col items-center gap-4 p-16'>
        <Spinner size={8} />
        <span className='text-xl font-medium'>
          {phase === 1
            ? 'Updating Display firmware'
            : phase === 2
              ? 'Updating Display filesystem'
              : phase === 3
                ? 'Updating controller firmware'
                : 'Finished'}
        </span>
        <span className='text-lg font-medium'>{phase === 4 ? 100 : progress}%</span>
        {phase === 4 && (
          <a href='/' className='btn btn-primary'>
            Back
          </a>
        )}
      </div>
    );
  }

  return (
    <>
      <div className='mb-4 flex flex-row items-center gap-2'>
        <h2 className='flex-grow text-2xl font-bold sm:text-3xl'>System & Updates</h2>
      </div>

      <form key='ota' method='post' action='/api/ota' ref={formRef} onSubmit={onSubmit}>
        <div className='grid grid-cols-1 gap-4 lg:grid-cols-12'>
          <Card sm={12} title='System Information'>
            {isLoading ? (
              <div className='flex w-full flex-row items-center justify-center py-16'>
                <Spinner size={8} />
              </div>
            ) : (
              <>
            <div className='flex flex-col space-y-4'>
              <label htmlFor='channel' className='mb-2 block text-sm font-medium'>
                Update Channel
              </label>
              <select id='channel' name='channel' className='select select-bordered w-full'>
                <option value='latest' selected={formData.channel === 'latest'}>
                  Stable
                </option>
                <option value='nightly' selected={formData.channel === 'nightly'}>
                  Nightly
                </option>
              </select>
            </div>

            <div className='grid grid-cols-1 md:grid-cols-3 gap-4 mt-4'>
              <div className='flex flex-col space-y-2 min-w-0'>
                <label className='text-sm font-medium'>Hardware</label>
                <div className='input input-bordered bg-base-200 cursor-default break-words whitespace-normal h-auto min-h-12 w-full'>
                  {formData.hardware}
                </div>
              </div>

              <div className='flex flex-col space-y-2 min-w-0'>
                <label className='text-sm font-medium'>Controller version</label>
                <div className='input input-bordered bg-base-200 cursor-default break-words whitespace-normal h-auto min-h-12 w-full'>
                  <span className='break-all'>{formData.controllerVersion}</span>
                  {formData.controllerUpdateAvailable && (
                    <span className='text-primary font-bold break-all'>
                      {' '}(Update available: {formData.latestVersion})
                    </span>
                  )}
                </div>
              </div>

              <div className='flex flex-col space-y-2 min-w-0'>
                <label className='text-sm font-medium'>Display version</label>
                <div className='input input-bordered bg-base-200 cursor-default break-words whitespace-normal h-auto min-h-12 w-full'>
                  <span className='break-all'>{formData.displayVersion}</span>
                  {formData.displayUpdateAvailable && (
                    <span className='text-primary font-bold break-all'>
                      {' '}(Update available: {formData.latestVersion})
                    </span>
                  )}
                </div>
              </div>
            </div>

            <div className='grid grid-cols-1 md:grid-cols-2 gap-4 mt-4'>
              {formData.spiffsTotal !== undefined && (
                <div className='flex flex-col space-y-2 min-w-0'>
                  <label className='text-sm font-medium'>Storage (SPIFFS)</label>
                  <div className='flex flex-col gap-1'>
                    <div className='bg-base-300 h-3 w-full overflow-hidden rounded'>
                      <div
                        className='bg-primary h-full transition-all'
                        style={{ width: `${formData.spiffsUsedPct || 0}%` }}
                      />
                    </div>
                    <div className='text-xs opacity-75'>
                      {((formData.spiffsUsed || 0) / 1024).toFixed(1)} KB /{' '}
                      {(formData.spiffsTotal / 1024).toFixed(1)} KB ({formData.spiffsUsedPct}%)
                    </div>
                  </div>
                </div>
              )}

              {formData.sdTotal !== undefined && (
                <div className='flex flex-col space-y-2 min-w-0'>
                  <label className='text-sm font-medium'>Storage (SD-Card)</label>
                  <div className='flex flex-col gap-1'>
                    <div className='bg-base-300 h-3 w-full overflow-hidden rounded'>
                      <div
                        className='bg-primary h-full transition-all'
                        style={{ width: `${formData.sdUsedPct || 0}%` }}
                      />
                    </div>
                    <div className='text-xs opacity-75'>
                      {((formData.sdUsed || 0) / 1024 / 1024).toFixed(1)} MB /{' '}
                      {(formData.sdTotal / 1024 / 1024).toFixed(1)} MB ({formData.sdUsedPct}%)
                    </div>
                  </div>
                </div>
              )}

              <div className='flex flex-col space-y-2 min-w-0'>
                <label className='text-sm font-medium'>Controller Signal Strength</label>
                <div className='flex flex-col gap-1'>
                  <div className='flex h-3 w-full gap-0.5'>
                    {[-80, -70, -60, -50].map((threshold, i) => (
                      <div key={i} className='bg-base-300 h-full flex-1 overflow-hidden rounded-sm'>
                        <div
                          className={`h-full ${rssi > threshold ? (rssi > -60 ? 'bg-success' : rssi > -70 ? 'bg-warning' : 'bg-error') : ''}`}
                          style={{ width: rssi > threshold ? '100%' : '0%' }}
                        />
                      </div>
                    ))}
                  </div>
                  <div className='text-xs opacity-75'>
                    {rssi} dBm ({rssi < -80 ? 'Poor' : rssi < -70 ? 'Weak' : rssi < -60 ? 'Fair' : rssi < -50 ? 'Good' : 'Excellent'})
                  </div>
                </div>
              </div>
            </div>

            <div className='alert alert-warning mt-2'>
              <span>
                Make sure to backup your profiles from the profile screen before updating the
                display.
              </span>
            </div>

            <div className='flex flex-col flex-wrap gap-2 sm:flex-row mt-2'>
              <button type='submit' className='btn btn-primary' disabled={submitting}>
                Save & Refresh
              </button>
              <button
                type='submit'
                name='update'
                className='btn btn-secondary'
                disabled={!formData.displayUpdateAvailable || submitting}
                onClick={() => onUpdate('display')}
              >
                Update Display
              </button>
              <button
                type='submit'
                name='update'
                className='btn btn-accent'
                disabled={!formData.controllerUpdateAvailable || submitting}
                onClick={() => onUpdate('controller')}
              >
                Update Controller
              </button>
              <button type='button' className='btn btn-outline' onClick={downloadSupportData}>
                Download Support Data
              </button>
              <button
                type='button'
                className='btn btn-outline'
                onClick={onHistoryRebuild}
                disabled={rebuilding}
              >
                Rebuild Shot History
                {rebuilding && (
                  <>
                    <Spinner size={4} className='ml-2' />
                    {rebuildProgress.total > 0 && (
                      <span className='ml-2 text-xs'>
                        {rebuildProgress.current}/{rebuildProgress.total}
                      </span>
                    )}
                  </>
                )}
                {rebuilt && (
                  <span className='text-success ml-2'>
                    <FontAwesomeIcon icon={faCheck}></FontAwesomeIcon>
                  </span>
                )}
              </button>
            </div>

            {rebuilding && (
              <div className='mt-3'>
                <div className='text-base-content/70 mb-1 text-sm'>
                  {rebuildProgress.status === 'starting' ||
                  rebuildProgress.status === 'scanning' ||
                  rebuildProgress.total === 0
                    ? 'Scanning shot history files...'
                    : `Processing shot history files (${rebuildProgress.current}/${rebuildProgress.total})`}
                </div>
                <div className='bg-base-300 h-2 w-full overflow-hidden rounded'>
                  <div
                    className={`h-full transition-all duration-300 ${
                      rebuildProgress.total === 0 ? 'bg-primary animate-pulse' : 'bg-primary'
                    }`}
                    style={{
                      width:
                        rebuildProgress.total > 0
                          ? `${(rebuildProgress.current / rebuildProgress.total) * 100}%`
                          : '30%',
                    }}
                  />
                </div>
              </div>
            )}
              </>
            )}
          </Card>

          <Card sm={12} title='Debug Logs'>
            <DebugLogs />
          </Card>
        </div>
      </form>
    </>
  );
}
