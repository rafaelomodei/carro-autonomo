'use client';

import StatusBar from '@/components/molecules/StatusBar';
import BottomBar from '@/components/organism/BottomBar';
import { usePathname } from 'next/navigation';

export default function PageLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const pathname = usePathname();
  const shouldTopAlign = pathname === '/settings' || pathname === '/debug';

  return (
    <div className='flex flex-col items-center h-full py-8'>
      <StatusBar />
      <div
        className={`flex flex-col w-full flex-grow items-center max-h-meddle ${
          shouldTopAlign ? '' : 'justify-center'
        }`}
      >
        {children}
      </div>
      <BottomBar />
    </div>
  );
}
