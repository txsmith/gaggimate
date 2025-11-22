import { useState, useEffect, useRef, useCallback, useContext } from 'preact/hooks';
import { Spinner } from '../../components/Spinner.jsx';
import { ApiServiceContext, machine } from '../../services/ApiService.js';
import Card from '../../components/Card.jsx';
import { downloadJson } from '../../utils/download.js';
import DebugLogs from '../../components/DebugLogs.jsx';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faArrowUp } from '@fortawesome/free-solid-svg-icons/faArrowUp';
import { faDownload } from '@fortawesome/free-solid-svg-icons/faDownload';

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
  const [checkingUpdates, setCheckingUpdates] = useState(false);
  const [otaData, setOtaData] = useState({});
  const [phase, setPhase] = useState(0);
  const [progress, setProgress] = useState(0);
  const [showingUpdateDialog, showUpdateDialog] = useState(null);
  const [downloadingProfiles, setDownloadingProfiles] = useState(false);
  const hasRequestedOtaSettings = useRef(false);
  const channelSwitchTimeoutRef = useRef(null);

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
      versions: otaData,
      coredump,
    };
    const ts = Date.now();
    downloadJson(supportFile, `support-${ts}.dat`);
  }, [otaData]);

  useEffect(() => {
    const listenerId = apiService.on('res:ota-settings', msg => {
      if (checkingUpdates) {
        setCheckingUpdates(false);
        if (channelSwitchTimeoutRef.current) {
          console.log('clearTimeout 2');
          clearTimeout(channelSwitchTimeoutRef.current);
          channelSwitchTimeoutRef.current = null;
        }
      }

      setOtaData(msg);
      setIsLoading(false);
    });
    return () => {
      apiService.off('res:ota-settings', listenerId);
    };
  }, [apiService, checkingUpdates]);

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
    // Wait for WebSocket to be connected before requesting OTA settings
    if (machine.value.connected && !hasRequestedOtaSettings.current) {
      hasRequestedOtaSettings.current = true;
      apiService.send({ tp: 'req:ota-settings' });
    }
  }, [apiService, machine.value.connected]);

  const changeChannel = useCallback(
    channel => {
      if (channel === otaData.channel) return;

      if (channelSwitchTimeoutRef.current) {
        console.log('clearTimeout 1');
        clearTimeout(channelSwitchTimeoutRef.current);
      }

      const previousChannel = otaData.channel;
      setOtaData({ ...otaData, channel });
      setCheckingUpdates(true);

      try {
        apiService.send({ tp: 'req:ota-settings', update: true, channel });
      } catch (error) {
        setCheckingUpdates(false);
        setOtaData(prev => ({ ...prev, channel: previousChannel }));
        throw error;
      }

      channelSwitchTimeoutRef.current = setTimeout(() => {
        console.log('Channel switch timeout fired, reverting to:', previousChannel);
        setCheckingUpdates(false);
        setOtaData(prev => ({ ...prev, channel: previousChannel }));
        channelSwitchTimeoutRef.current = null;
      }, 10000);
    },
    [apiService, otaData],
  );

  const confirmUpdate = useCallback(
    component => {
      apiService.send({ tp: 'req:ota-start', cp: component });
      showUpdateDialog(null);
    },
    [apiService],
  );

  const downloadProfiles = useCallback(async () => {
    setDownloadingProfiles(true);
    try {
      const response = await apiService.request({ tp: 'req:profiles:list' });
      const exportedProfiles = response.profiles.map(p => {
        const ep = { ...p };
        delete ep.id;
        delete ep.selected;
        delete ep.favorite;
        return ep;
      });
      downloadJson(exportedProfiles, 'profiles.json');
    } finally {
      setDownloadingProfiles(false);
    }
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

  const displayUpdateAvailable = otaData.displayUpdateAvailable;
  const controllerUpdateAvailable = otaData.controllerUpdateAvailable;

  return (
    <>
      <div className='grid grid-cols-1 gap-4 lg:grid-cols-12'>
        <Card sm={12} title='System Information'>
          {isLoading ? (
            <div className='flex w-full flex-row items-center justify-center py-16'>
              <Spinner size={8} />
            </div>
          ) : (
            <>
              {/* Update channel selector */}
              <div className='flex flex-col space-y-2'>
                <label className='text-sm font-medium'>Update Channel</label>
                <div className='flex items-center gap-3'>
                  <div className='bg-base-300 flex w-full max-w-xs rounded-full p-1'>
                    <button
                      type='button'
                      className={`flex-1 cursor-pointer rounded-full px-4 py-2 text-sm transition-all duration-200 ${
                        otaData.channel === 'latest'
                          ? 'bg-primary text-primary-content font-medium'
                          : 'text-base-content/60 hover:text-base-content'
                      }`}
                      onClick={() => changeChannel('latest')}
                      disabled={checkingUpdates}
                    >
                      Stable
                    </button>
                    <button
                      type='button'
                      className={`flex-1 cursor-pointer rounded-full px-4 py-2 text-sm transition-all duration-200 ${
                        otaData.channel === 'nightly'
                          ? 'bg-primary text-primary-content font-medium'
                          : 'text-base-content/60 hover:text-base-content'
                      }`}
                      onClick={() => changeChannel('nightly')}
                      disabled={checkingUpdates}
                    >
                      Nightly
                    </button>
                  </div>
                  {checkingUpdates && <Spinner size={4} />}
                </div>
              </div>

              {/* Hardware & firmware version info */}
              <div className='mt-2 grid grid-cols-1 gap-4 md:grid-cols-3'>
                <div className='flex min-w-0 flex-col space-y-2'>
                  <label className='text-sm font-medium'>Hardware</label>
                  <div className='input input-bordered bg-base-200 h-auto min-h-12 w-full cursor-default break-words whitespace-normal'>
                    {otaData.hardware}
                  </div>
                </div>

                <div className='flex min-w-0 flex-col space-y-2'>
                  <label className='text-sm font-medium'>Controller version</label>
                  <div className='relative flex items-stretch'>
                    <div
                      className={`input bg-base-200 h-auto min-h-12 flex-1 cursor-default pr-11 break-words whitespace-normal ${controllerUpdateAvailable ? 'border-primary' : 'input-bordered'}`}
                    >
                      <span className='break-all'>{otaData.controllerVersion}</span>
                    </div>
                    {controllerUpdateAvailable && (
                      <button
                        type='button'
                        className='btn btn-primary btn-square absolute top-[1px] right-[1px] bottom-[1px] h-auto'
                        onClick={() => showUpdateDialog('controller')}
                        title={otaData.latestVersion}
                      >
                        <FontAwesomeIcon icon={faArrowUp} />
                      </button>
                    )}
                  </div>
                </div>

                <div className='flex min-w-0 flex-col space-y-2'>
                  <label className='text-sm font-medium'>Display version</label>
                  <div className='relative flex items-stretch'>
                    <div
                      className={`input bg-base-200 h-auto min-h-12 flex-1 cursor-default pr-11 break-words whitespace-normal ${displayUpdateAvailable ? 'border-primary' : 'input-bordered'}`}
                    >
                      <span className='break-all'>{otaData.displayVersion}</span>
                    </div>
                    {displayUpdateAvailable && (
                      <button
                        type='button'
                        className='btn btn-primary btn-square absolute top-[1px] right-[1px] bottom-[1px] h-auto'
                        onClick={() => showUpdateDialog('display')}
                        title={otaData.latestVersion}
                      >
                        <FontAwesomeIcon icon={faArrowUp} />
                      </button>
                    )}
                  </div>
                </div>
              </div>

              {/* SPIFFS / SD-Card usage */}
              <div className='mt-4 grid grid-cols-1 gap-4 md:grid-cols-2'>
                {otaData.spiffsTotal !== undefined && (
                  <div className='flex min-w-0 flex-col space-y-2'>
                    <label className='text-sm font-medium'>Storage (SPIFFS)</label>
                    <div className='flex flex-col gap-1'>
                      <div className='bg-base-300 h-3 w-full overflow-hidden rounded'>
                        <div
                          className='bg-primary h-full transition-all'
                          style={{ width: `${otaData.spiffsUsedPct || 0}%` }}
                        />
                      </div>
                      <div className='text-xs opacity-75'>
                        {((otaData.spiffsUsed || 0) / 1024).toFixed(1)} KB /{' '}
                        {(otaData.spiffsTotal / 1024).toFixed(1)} KB ({otaData.spiffsUsedPct}%)
                      </div>
                    </div>
                  </div>
                )}

                {otaData.sdTotal !== undefined && (
                  <div className='flex min-w-0 flex-col space-y-2'>
                    <label className='text-sm font-medium'>Storage (SD-Card)</label>
                    <div className='flex flex-col gap-1'>
                      <div className='bg-base-300 h-3 w-full overflow-hidden rounded'>
                        <div
                          className='bg-primary h-full transition-all'
                          style={{ width: `${otaData.sdUsedPct || 0}%` }}
                        />
                      </div>
                      <div className='text-xs opacity-75'>
                        {((otaData.sdUsed || 0) / 1024 / 1024).toFixed(1)} MB /{' '}
                        {(otaData.sdTotal / 1024 / 1024).toFixed(1)} MB ({otaData.sdUsedPct}%)
                      </div>
                    </div>
                  </div>
                )}
              </div>

              <div className='mt-4 flex flex-col flex-wrap gap-2 sm:flex-row'>
                <button type='button' className='btn btn-soft' onClick={downloadSupportData}>
                  <FontAwesomeIcon icon={faDownload} />
                  Download Support Data
                </button>
              </div>
            </>
          )}
        </Card>

        <Card sm={12} title='Debug Logs'>
          <DebugLogs />
        </Card>
      </div>

      {showingUpdateDialog === 'display' && (
        <div className='modal modal-open' onClick={() => showUpdateDialog(null)}>
          <div className='modal-box' onClick={e => e.stopPropagation()}>
            <h3 className='mb-4 text-lg font-bold'>Confirm Update</h3>
            <p className='py-4'>
              <strong>{otaData.displayVersion}</strong> →{' '}
              <strong>{otaData.latestVersion || 'v1.2.3'}</strong>
              <br />
              <br />
              Are you sure you want to update the display firmware? Make sure to backup your
              profiles before updating the display.
            </p>
            <div className='mb-4'>
              <button
                className='btn btn-warning btn-sm w-full'
                onClick={downloadProfiles}
                disabled={downloadingProfiles}
              >
                {downloadingProfiles ? <Spinner size={4} /> : <FontAwesomeIcon icon={faDownload} />}
                Download Profiles
              </button>
            </div>
            <div className='modal-action'>
              <button className='btn' onClick={() => showUpdateDialog(null)}>
                Cancel
              </button>
              <button className='btn btn-primary' onClick={() => confirmUpdate('display')}>
                Continue Update
              </button>
            </div>
          </div>
        </div>
      )}

      {showingUpdateDialog === 'controller' && (
        <div className='modal modal-open' onClick={() => showUpdateDialog(null)}>
          <div className='modal-box' onClick={e => e.stopPropagation()}>
            <h3 className='mb-4 text-lg font-bold'>Confirm Update</h3>
            <p className='py-4'>
              <strong>{otaData.controllerVersion}</strong> →{' '}
              <strong>{otaData.latestVersion || 'v1.2.3'}</strong>
              <br />
              <br />
              Are you sure you want to update the controller firmware?
            </p>
            <div className='modal-action'>
              <button className='btn' onClick={() => showUpdateDialog(null)}>
                Cancel
              </button>
              <button className='btn btn-primary' onClick={() => confirmUpdate('controller')}>
                Continue Update
              </button>
            </div>
          </div>
        </div>
      )}
    </>
  );
}
