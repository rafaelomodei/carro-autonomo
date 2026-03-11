'use client';

import { useEffect } from 'react';
import { emitUiLogEvent } from '@/lib/vehicle-events';
import { useVehicleConfig } from '@/providers/VehicleConfigProvider';
import { useVehicleLogging } from '@/providers/VehicleLoggingProvider';

const isInteractiveElement = (target: EventTarget | null) => {
  if (!(target instanceof HTMLElement)) {
    return false;
  }

  return (
    target.tagName === 'INPUT' ||
    target.tagName === 'TEXTAREA' ||
    target.tagName === 'SELECT' ||
    target.isContentEditable
  );
};

const GlobalVehicleShortcuts = () => {
  const {
    beginManualMotionHold,
    beginManualSteeringHold,
    effectiveDrivingMode,
    endManualMotionHold,
    endManualSteeringHold,
    startAutonomous,
    stopAutonomous,
    stopManualControl,
    toggleDrivingMode,
  } = useVehicleConfig();
  const { recordingState, toggleRecording } = useVehicleLogging();

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.repeat || isInteractiveElement(event.target)) {
        return;
      }

      const key = event.key.toLowerCase();

      switch (key) {
        case 'o':
          event.preventDefault();
          emitUiLogEvent('shortcut.recording_toggle', {
            next_action: recordingState === 'recording' ? 'stop' : 'start',
          });
          void toggleRecording();
          break;
        case 'm':
          event.preventDefault();
          emitUiLogEvent('shortcut.mode_toggle', {
            current_mode: effectiveDrivingMode,
          });
          toggleDrivingMode();
          break;
        case 'w':
          event.preventDefault();

          if (effectiveDrivingMode === 'autonomous') {
            emitUiLogEvent('shortcut.autonomous_start', null);
            startAutonomous();
            return;
          }

          beginManualMotionHold('forward');
          break;
        case 's':
          event.preventDefault();

          if (effectiveDrivingMode === 'autonomous') {
            emitUiLogEvent('shortcut.autonomous_stop', null);
            stopAutonomous();
            return;
          }

          beginManualMotionHold('backward');
          break;
        case 'a':
          if (effectiveDrivingMode !== 'manual') {
            return;
          }

          event.preventDefault();
          beginManualSteeringHold('left');
          break;
        case 'd':
          if (effectiveDrivingMode !== 'manual') {
            return;
          }

          event.preventDefault();
          beginManualSteeringHold('right');
          break;
        case ' ':
          if (effectiveDrivingMode !== 'manual') {
            return;
          }

          event.preventDefault();
          stopManualControl();
          break;
        default:
          break;
      }
    };

    const handleKeyUp = (event: KeyboardEvent) => {
      if (isInteractiveElement(event.target)) {
        return;
      }

      const key = event.key.toLowerCase();

      if (effectiveDrivingMode !== 'manual') {
        return;
      }

      switch (key) {
        case 'w':
        case 's':
          endManualMotionHold();
          break;
        case 'a':
        case 'd':
          endManualSteeringHold();
          break;
        default:
          break;
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);

    return () => {
      window.removeEventListener('keydown', handleKeyDown);
      window.removeEventListener('keyup', handleKeyUp);
    };
  }, [
    beginManualMotionHold,
    beginManualSteeringHold,
    effectiveDrivingMode,
    endManualMotionHold,
    endManualSteeringHold,
    recordingState,
    startAutonomous,
    stopAutonomous,
    stopManualControl,
    toggleDrivingMode,
    toggleRecording,
  ]);

  return null;
};

export default GlobalVehicleShortcuts;
