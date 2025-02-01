import type { Config } from 'tailwindcss';

const config: Config = {
  darkMode: ['class'],
  content: [
    './src/pages/**/*.{js,ts,jsx,tsx,mdx}',
    './src/components/**/*.{js,ts,jsx,tsx,mdx}',
    './src/**/*.{js,ts,jsx,tsx}',
  ],
  theme: {
    extend: {
      maxHeight: {
        meddle: 'calc(100vh - 152px)',
      },
      colors: {
        background: {
          DEFAULT: '#121212',
          light: '#ffffff',
        },
        foreground: {
          DEFAULT: '#ffffff',
          light: '#1a1a1a',
        },
        primary: {
          DEFAULT: '#27272A',
          light: '#505054FF',
        },
        secondary: {
          DEFAULT: '#A9A9A9',
          light: '#d3d3d3',
        },
        accent: '#00FF00',
      },
      borderRadius: {
        lg: 'var(--radius)',
        md: 'calc(var(--radius) - 2px)',
        sm: 'calc(var(--radius) - 4px)',
      },
      keyframes: {
        fadeIn: {
          '0%': { opacity: '0' },
          '100%': { opacity: '1' },
        },
        fadeOut: {
          '0%': { opacity: '1' },
          '100%': { opacity: '0' },
        },
        textGlow: {
          '0%': { backgroundPosition: '0 -200%' },
          '100%': { backgroundPosition: '0 200%' },
        },
        pulseGrow: {
          '0%, 100%': { transform: 'scale(1)' },
          '50%': { transform: 'scale(1.2)' },
        },
      },
      animation: {
        fadeIn: 'fadeIn 1s ease-in-out',
        fadeOut: 'fadeOut 1s ease-in-out',
        'text-glow': 'textGlow 4s linear infinite',
        'pulse-grow': 'pulseGrow 2s ease-in-out infinite',
      },
    },
  },
  plugins: [require('tailwindcss-animate')],
};

export default config;
