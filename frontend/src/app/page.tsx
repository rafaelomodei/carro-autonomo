'use client';

import { useState, useEffect } from 'react';
import { signIn } from 'next-auth/react';
import { useRouter } from 'next/navigation';
import { ModeToggle } from '@/components/atomics/ModeToggle';
import { Input } from '@/components/ui/input';
import { Button } from '@/components/ui/button';
import { Label } from '@/components/ui/label';
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card';
import { AiOutlineEye, AiOutlineEyeInvisible } from 'react-icons/ai';

const LoginPage = () => {
  const router = useRouter();
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [isPasswordVisible, setIsPasswordVisible] = useState(false);
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [fadeIn, setFadeIn] = useState(false);
  const [fadeOut, setFadeOut] = useState(false);

  useEffect(() => {
    const fadeInTimer = setTimeout(() => {
      setFadeIn(true);
    }, 100);

    return () => {
      clearTimeout(fadeInTimer);
    };
  }, []);

  const togglePasswordVisibility = () => {
    setIsPasswordVisible(!isPasswordVisible);
  };

  const handleLogin = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsLoading(true);

    const result = await signIn('credentials', {
      redirect: false,
      email,
      password,
    });

    if (result?.error) {
      setError(result.error);
      setIsLoading(false);
      return;
    }

    setFadeOut(true);
    setTimeout(() => {
      router.push('/welcome');
    }, 1000);
  };

  return (
    <div
      className={`flex items-center justify-center min-h-screen transition-opacity duration-1000 ${
        fadeIn && !fadeOut ? 'opacity-100' : 'opacity-0'
      }`}
    >
      <div className='absolute top-4 right-4'>
        <ModeToggle />
      </div>
      <Card className='w-full max-w-sm border-none shadow-none'>
        <CardHeader>
          <CardTitle className='text-center'>Entrar</CardTitle>
        </CardHeader>
        <CardContent>
          <form onSubmit={handleLogin}>
            {error && <p className='mb-4 text-red-500'>{error}</p>}
            <div className='mb-4'>
              <Label htmlFor='email' className='block mb-2'>
                Email
              </Label>
              <Input
                type='email'
                id='email'
                value={email}
                onChange={(e) => setEmail(e.target.value)}
                required
              />
            </div>
            <div className='mb-6 relative'>
              <Label htmlFor='password' className='block mb-2'>
                Senha
              </Label>
              <Input
                type={isPasswordVisible ? 'text' : 'password'}
                id='password'
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                required
              />
              <div
                className='absolute inset-y-0 right-3 top-6 flex items-center cursor-pointer'
                onClick={togglePasswordVisibility}
              >
                {isPasswordVisible ? (
                  <AiOutlineEyeInvisible size={20} />
                ) : (
                  <AiOutlineEye size={20} />
                )}
              </div>
            </div>
            <Button type='submit' className='w-full' disabled={isLoading}>
              {isLoading ? 'Carregando...' : 'Entrar'}
            </Button>
          </form>
        </CardContent>
        <p className='pt-4 text-center text-xs text-gray-500'>
          Desenvolvido por{' '}
          <a
            className='font-bold'
            href='https://github.com/rafaelomodei'
            target='_blank'
          >
            Rafael Omodei
          </a>
        </p>
      </Card>
    </div>
  );
};

export default LoginPage;
