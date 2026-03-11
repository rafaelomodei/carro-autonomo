import { NextRequest, NextResponse } from 'next/server';
import { finalizeSession } from '@/lib/logging/server';
import { SessionFinishRequest } from '@/lib/logging/shared';

export const runtime = 'nodejs';

export async function POST(
  request: NextRequest,
  { params }: { params: { sessionId: string } }
) {
  try {
    const payload = (await request.json()) as SessionFinishRequest;
    const sessionSummary = await finalizeSession(params.sessionId, payload);

    return NextResponse.json(sessionSummary);
  } catch (error) {
    const message =
      error instanceof Error ? error.message : 'Falha ao finalizar sessao de logging.';

    return NextResponse.json({ error: message }, { status: 500 });
  }
}
