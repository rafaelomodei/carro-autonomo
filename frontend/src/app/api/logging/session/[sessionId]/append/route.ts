import { NextRequest, NextResponse } from 'next/server';
import { appendSessionData } from '@/lib/logging/server';
import { AppendSessionRequest } from '@/lib/logging/shared';

export const runtime = 'nodejs';

export async function POST(
  request: NextRequest,
  { params }: { params: { sessionId: string } }
) {
  try {
    const payload = (await request.json()) as AppendSessionRequest;
    await appendSessionData(params.sessionId, payload);

    return NextResponse.json({ ok: true });
  } catch (error) {
    const message =
      error instanceof Error ? error.message : 'Falha ao gravar dados da sessao.';

    return NextResponse.json({ error: message }, { status: 500 });
  }
}
