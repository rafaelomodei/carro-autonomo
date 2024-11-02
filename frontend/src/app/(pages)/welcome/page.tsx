'use client';

import { useEffect, useState } from 'react';
import { useSession, signIn } from 'next-auth/react';
import { useRouter } from 'next/navigation';
import WelcomeMessage from '@/components/organism/WelcomeMessage';

const Welcome = () => {
  const { data: session, status } = useSession();
  const router = useRouter();
  const [fadeIn, setFadeIn] = useState(false);
  const [fadeOut, setFadeOut] = useState(false);

  useEffect(() => {
    if (status === 'unauthenticated') {
      signIn();
    }

    const fadeInTimer = setTimeout(() => {
      setFadeIn(true);
    }, 100);

    const redirectTimer = setTimeout(() => {
      setFadeOut(true);
      setTimeout(() => {
        router.push('/home');
      }, 1000);
    }, 5000);

    return () => {
      clearTimeout(fadeInTimer);
      clearTimeout(redirectTimer);
    };
  }, [status, router]);

  if (status === 'loading') {
    return <div>Carregando...</div>;
  }

  return (
    <div
      className={`flex items-center justify-center min-h-screen transition-opacity duration-1000 ${
        fadeIn && !fadeOut ? 'opacity-100' : 'opacity-0'
      } ${fadeOut ? 'opacity-0' : ''}`}
    >
      <WelcomeMessage name={session?.user?.name || 'Convidado'} />
    </div>
  );
};

export default Welcome;
