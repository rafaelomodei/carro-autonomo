import { NextRequest, NextResponse } from 'next/server';
import { appendVideoChunk } from '@/lib/logging/server';
import { RECORDING_VIEW_IDS, RecordingViewId } from '@/lib/logging/shared';

export const runtime = 'nodejs';

const isRecordingView = (value: string): value is RecordingViewId =>
  RECORDING_VIEW_IDS.includes(value as RecordingViewId);

export async function POST(
  request: NextRequest,
  {
    params,
  }: {
    params: { sessionId: string; view: string };
  }
) {
  try {
    if (!isRecordingView(params.view)) {
      return NextResponse.json({ error: 'View de video invalida.' }, { status: 400 });
    }

    const body = await request.arrayBuffer();
    await appendVideoChunk(
      params.sessionId,
      params.view,
      Buffer.from(body)
    );

    return NextResponse.json({ ok: true });
  } catch (error) {
    const message =
      error instanceof Error ? error.message : 'Falha ao anexar chunk de video.';

    return NextResponse.json({ error: message }, { status: 500 });
  }
}
