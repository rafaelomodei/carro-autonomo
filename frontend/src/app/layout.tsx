import './globals.css';
import type { Metadata } from 'next';
import NextAuthSessionProvider from '@/providers/nextAuthSessionProvider';
import { ThemeProvider } from '@/providers/themeProvider';
import { DestinationProvider } from '@/providers/DestinationProvider';
import { VehicleConfigProvider } from '@/providers/VehicleConfigProvider';

export const metadata: Metadata = {
  title: 'Carro autônomo',
  description: 'Desenvolvido por Rafael Omodei',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang='pt-BR' suppressHydrationWarning>
      <body className='md:px-4 lg:px-8'>
        <ThemeProvider
          attribute='class'
          defaultTheme='system'
          enableSystem
          disableTransitionOnChange
        >
          <NextAuthSessionProvider>
            <DestinationProvider>
              <VehicleConfigProvider>{children}</VehicleConfigProvider>
            </DestinationProvider>
          </NextAuthSessionProvider>
        </ThemeProvider>
      </body>
    </html>
  );
}
