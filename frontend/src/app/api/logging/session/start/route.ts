import { NextRequest, NextResponse } from 'next/server';
import {
  initializeSession,
} from '@/lib/logging/server';
import {
  RECORDING_VIEW_IDS,
  SessionStartRequest,
} from '@/lib/logging/shared';

export const runtime = 'nodejs';

const isValidStartRequest = (payload: SessionStartRequest) =>
  typeof payload?.ws_url === 'string' &&
  Array.isArray(payload.recorded_views) &&
  Array.isArray(payload.user_selected_views_at_start) &&
  payload.recorded_views.every((view) => RECORDING_VIEW_IDS.includes(view)) &&
  payload.user_selected_views_at_start.every((view) =>
    RECORDING_VIEW_IDS.includes(view)
  );

export async function POST(request: NextRequest) {
  try {
    const payload = (await request.json()) as SessionStartRequest;

    if (!isValidStartRequest(payload)) {
      return NextResponse.json(
        { error: 'Payload invalido para iniciar sessao de logging.' },
        { status: 400 }
      );
    }

    const session = await initializeSession(payload);
    return NextResponse.json(session);
  } catch (error) {
    const message =
      error instanceof Error ? error.message : 'Falha ao iniciar sessao de logging.';

    return NextResponse.json({ error: message }, { status: 500 });
  }
}
