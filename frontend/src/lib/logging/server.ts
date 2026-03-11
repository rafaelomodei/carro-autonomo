import { spawn, spawnSync } from 'node:child_process';
import {
  appendFile,
  mkdir,
  readFile,
  rm,
  stat,
  writeFile,
} from 'node:fs/promises';
import path from 'node:path';
import {
  AUTONOMOUS_CONTROL_CSV_HEADERS,
  AppendSessionRequest,
  RecordingViewId,
  ROAD_SEGMENTATION_CSV_HEADERS,
  serializeCsvRow,
  SessionConversionStatus,
  SessionFinishRequest,
  SessionStartRequest,
  SessionStartResponse,
  SessionVideoOutput,
  UiLogEventRecord,
} from '@/lib/logging/shared';

const SESSION_ID_PATTERN = /^[a-zA-Z0-9_-]+$/;

const buildSessionId = () => {
  const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
  const randomSuffix = Math.random().toString(36).slice(2, 8);
  return `${timestamp}_${randomSuffix}`;
};

export const getFrontendLogDataDir = () =>
  process.env.FRONTEND_LOG_DATA_DIR?.trim()
    ? path.resolve(process.env.FRONTEND_LOG_DATA_DIR)
    : path.resolve(process.cwd(), '..', 'data');

export const resolveSessionDir = (sessionId: string) => {
  if (!SESSION_ID_PATTERN.test(sessionId)) {
    throw new Error(`Session id invalido: ${sessionId}`);
  }

  return path.join(getFrontendLogDataDir(), sessionId);
};

const resolveSessionFile = (sessionId: string, ...segments: string[]) =>
  path.join(resolveSessionDir(sessionId), ...segments);

const ensureCsvFile = async (filePath: string, headers: readonly string[]) => {
  await writeFile(filePath, `${headers.join(',')}\n`, { flag: 'wx' }).catch(
    async (error: NodeJS.ErrnoException) => {
      if (error.code !== 'EEXIST') {
        throw error;
      }
    }
  );
};

export const ensureFfmpegAvailable = () => {
  const result = spawnSync('ffmpeg', ['-version'], { stdio: 'ignore' });
  return result.status === 0;
};

export const initializeSession = async (
  input: SessionStartRequest
): Promise<SessionStartResponse> => {
  if (!ensureFfmpegAvailable()) {
    throw new Error('ffmpeg nao esta disponivel no PATH do frontend.');
  }

  const sessionId = buildSessionId();
  const sessionDir = resolveSessionDir(sessionId);

  await mkdir(resolveSessionFile(sessionId, 'telemetry'), { recursive: true });
  await mkdir(resolveSessionFile(sessionId, 'events'), { recursive: true });
  await mkdir(resolveSessionFile(sessionId, 'vision'), { recursive: true });
  await mkdir(resolveSessionFile(sessionId, 'videos'), { recursive: true });
  await mkdir(resolveSessionFile(sessionId, '.tmp'), { recursive: true });

  await writeFile(resolveSessionFile(sessionId, 'telemetry', 'raw.ndjson'), '', {
    flag: 'w',
  });
  await writeFile(resolveSessionFile(sessionId, 'events', 'ui.ndjson'), '', {
    flag: 'w',
  });
  await writeFile(resolveSessionFile(sessionId, 'vision', 'frame_index.ndjson'), '', {
    flag: 'w',
  });

  await ensureCsvFile(
    resolveSessionFile(sessionId, 'telemetry', 'road_segmentation_timeseries.csv'),
    ROAD_SEGMENTATION_CSV_HEADERS
  );
  await ensureCsvFile(
    resolveSessionFile(sessionId, 'telemetry', 'autonomous_control_timeseries.csv'),
    AUTONOMOUS_CONTROL_CSV_HEADERS
  );

  await writeFile(
    resolveSessionFile(sessionId, 'session.json'),
    `${JSON.stringify(
      {
        session_id: sessionId,
        started_at_ms: Date.now(),
        ws_url: input.ws_url,
        recorded_views: input.recorded_views,
        user_selected_views_at_start: input.user_selected_views_at_start,
        stop_reason: null,
        automatic_stop: false,
        telemetry_counts: null,
        video_outputs: null,
        conversion_status: null,
        errors: [],
      },
      null,
      2
    )}\n`,
    { flag: 'w' }
  );

  return {
    session_id: sessionId,
    session_dir: sessionDir,
  };
};

interface StoredSessionMetadata {
  session_id: string;
  started_at_ms: number;
  ws_url: string;
  recorded_views: RecordingViewId[];
  user_selected_views_at_start: RecordingViewId[];
}

export const readSessionMetadata = async (
  sessionId: string
): Promise<StoredSessionMetadata> => {
  const sessionFile = await readFile(resolveSessionFile(sessionId, 'session.json'), 'utf-8');
  const parsed = JSON.parse(sessionFile) as StoredSessionMetadata;

  return parsed;
};

const appendJsonLines = async (filePath: string, items: unknown[]) => {
  if (items.length === 0) {
    return;
  }

  await appendFile(
    filePath,
    items.map((item) => JSON.stringify(item)).join('\n').concat('\n')
  );
};

export const appendSessionData = async (
  sessionId: string,
  payload: AppendSessionRequest
) => {
  if (payload.telemetry_raw?.length) {
    await appendJsonLines(
      resolveSessionFile(sessionId, 'telemetry', 'raw.ndjson'),
      payload.telemetry_raw
    );
  }

  if (payload.ui_events?.length) {
    await appendJsonLines(
      resolveSessionFile(sessionId, 'events', 'ui.ndjson'),
      payload.ui_events
    );
  }

  if (payload.frame_index?.length) {
    await appendJsonLines(
      resolveSessionFile(sessionId, 'vision', 'frame_index.ndjson'),
      payload.frame_index
    );
  }

  if (payload.road_segmentation_rows?.length) {
    await appendFile(
      resolveSessionFile(sessionId, 'telemetry', 'road_segmentation_timeseries.csv'),
      payload.road_segmentation_rows
        .map((row) => serializeCsvRow(ROAD_SEGMENTATION_CSV_HEADERS, row))
        .join('')
    );
  }

  if (payload.autonomous_control_rows?.length) {
    await appendFile(
      resolveSessionFile(sessionId, 'telemetry', 'autonomous_control_timeseries.csv'),
      payload.autonomous_control_rows
        .map((row) => serializeCsvRow(AUTONOMOUS_CONTROL_CSV_HEADERS, row))
        .join('')
    );
  }
};

export const appendVideoChunk = async (
  sessionId: string,
  view: RecordingViewId,
  chunk: Buffer
) => {
  await appendFile(resolveSessionFile(sessionId, '.tmp', `${view}.webm`), chunk);
};

const getVideoBytes = async (filePath: string) => {
  try {
    const fileStats = await stat(filePath);
    return fileStats.size;
  } catch (error) {
    const fileError = error as NodeJS.ErrnoException;
    if (fileError.code === 'ENOENT') {
      return 0;
    }

    throw error;
  }
};

const convertWebmToMp4 = (inputPath: string, outputPath: string) =>
  new Promise<void>((resolve, reject) => {
    const ffmpegProcess = spawn(
      'ffmpeg',
      [
        '-y',
        '-i',
        inputPath,
        '-an',
        '-c:v',
        'libx264',
        '-pix_fmt',
        'yuv420p',
        outputPath,
      ],
      {
        stdio: ['ignore', 'ignore', 'pipe'],
      }
    );

    let stderr = '';

    ffmpegProcess.stderr.on('data', (chunk) => {
      stderr += chunk.toString();
    });

    ffmpegProcess.on('error', (error) => {
      reject(error);
    });

    ffmpegProcess.on('close', (code) => {
      if (code === 0) {
        resolve();
        return;
      }

      reject(new Error(stderr || `ffmpeg encerrou com codigo ${code ?? 'desconhecido'}.`));
    });
  });

const createInitialConversionStatus = (
  views: RecordingViewId[]
): SessionConversionStatus['views'] =>
  views.reduce(
    (accumulator, view) => ({
      ...accumulator,
      [view]: {
        status: 'missing',
      },
    }),
    {} as SessionConversionStatus['views']
  );

const countConversionFailures = (statuses: SessionConversionStatus['views']) =>
  Object.values(statuses).filter((status) => status.status === 'failed').length;

const countConvertedVideos = (statuses: SessionConversionStatus['views']) =>
  Object.values(statuses).filter((status) => status.status === 'converted').length;

export const finalizeSession = async (
  sessionId: string,
  finishInput: SessionFinishRequest
) => {
  const storedSession = await readSessionMetadata(sessionId);
  const conversionViews = createInitialConversionStatus(storedSession.recorded_views);
  const videoOutputs = {} as Record<RecordingViewId, SessionVideoOutput>;
  const errors = [...finishInput.errors];

  for (const view of storedSession.recorded_views) {
    const tmpPath = resolveSessionFile(sessionId, '.tmp', `${view}.webm`);
    const mp4Path = resolveSessionFile(sessionId, 'videos', `${view}.mp4`);
    const relativeTmpPath = path.relative(resolveSessionDir(sessionId), tmpPath);
    const relativeMp4Path = path.relative(resolveSessionDir(sessionId), mp4Path);
    const bytesWritten = await getVideoBytes(tmpPath);

    videoOutputs[view] = {
      view,
      mp4_path: null,
      temporary_webm_path: bytesWritten > 0 ? relativeTmpPath : null,
      bytes_written: bytesWritten,
      converted: false,
    };

    if (bytesWritten === 0) {
      conversionViews[view] = {
        status: 'missing',
        message: 'Nenhum chunk WEBM foi recebido para esta view.',
      };
      continue;
    }

    try {
      await convertWebmToMp4(tmpPath, mp4Path);
      await rm(tmpPath, { force: true });

      conversionViews[view] = {
        status: 'converted',
      };
      videoOutputs[view] = {
        ...videoOutputs[view],
        mp4_path: relativeMp4Path,
        temporary_webm_path: null,
        converted: true,
      };
    } catch (error) {
      const message =
        error instanceof Error ? error.message : 'Falha desconhecida ao converter video.';
      conversionViews[view] = {
        status: 'failed',
        message,
      };
      errors.push(`[${view}] ${message}`);
    }
  }

  const failedCount = countConversionFailures(conversionViews);
  const convertedCount = countConvertedVideos(conversionViews);
  const conversionStatus: SessionConversionStatus = {
    overall:
      failedCount > 0
        ? convertedCount > 0
          ? 'partial'
          : 'failed'
        : 'completed',
    views: conversionViews,
  };

  const sessionJson = {
    session_id: sessionId,
    started_at_ms: storedSession.started_at_ms,
    ended_at_ms: finishInput.ended_at_ms,
    duration_ms: Math.max(0, finishInput.ended_at_ms - storedSession.started_at_ms),
    ws_url: storedSession.ws_url,
    recorded_views: storedSession.recorded_views,
    user_selected_views_at_start: storedSession.user_selected_views_at_start,
    stop_reason: finishInput.stop_reason,
    automatic_stop: finishInput.automatic_stop,
    telemetry_counts: finishInput.telemetry_counts,
    video_outputs: videoOutputs,
    conversion_status: conversionStatus,
    errors,
  };

  await writeFile(
    resolveSessionFile(sessionId, 'session.json'),
    `${JSON.stringify(sessionJson, null, 2)}\n`,
    { flag: 'w' }
  );

  return sessionJson;
};

export const appendUiLogEvent = async (sessionId: string, event: UiLogEventRecord) => {
  await appendJsonLines(resolveSessionFile(sessionId, 'events', 'ui.ndjson'), [event]);
};
