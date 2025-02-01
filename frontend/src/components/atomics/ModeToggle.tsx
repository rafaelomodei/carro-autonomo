'use client';

import { useState, useEffect } from 'react';
import { Moon, Sun, Monitor } from 'lucide-react';
import { useTheme } from 'next-themes';
import { Button } from '@/components/ui/button';
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from '@/components/ui/dropdown-menu';

interface ModeToggleProps {
  showSelectedTheme?: boolean;
}

export function ModeToggle({ showSelectedTheme = false }: ModeToggleProps) {
  const { theme, setTheme } = useTheme();
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    setMounted(true);
  }, []);

  if (!mounted) return null; // Evita erro de hidratação

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button variant='outline' className='flex gap-4'>
          {theme === 'light' && <Sun className='h-[1.2rem] w-[1.2rem]' />}
          {theme === 'dark' && <Moon className='h-[1.2rem] w-[1.2rem]' />}
          {theme === 'system' && <Monitor className='h-[1.2rem] w-[1.2rem]' />}
          {showSelectedTheme && (
            <div className='text-sm'>
              {theme
                ? theme.charAt(0).toUpperCase() + theme.slice(1)
                : 'Desconhecido'}
            </div>
          )}
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent align='end'>
        <DropdownMenuItem onClick={() => setTheme('light')}>
          Light
        </DropdownMenuItem>
        <DropdownMenuItem onClick={() => setTheme('dark')}>
          Dark
        </DropdownMenuItem>
        <DropdownMenuItem onClick={() => setTheme('system')}>
          System
        </DropdownMenuItem>
      </DropdownMenuContent>
    </DropdownMenu>
  );
}
