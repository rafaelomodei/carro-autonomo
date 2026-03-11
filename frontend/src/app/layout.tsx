import './globals.css';
import type { Metadata } from 'next';
import NextAuthSessionProvider from '@/providers/nextAuthSessionProvider';
import { ThemeProvider } from '@/providers/themeProvider';
import { DestinationProvider } from '@/providers/DestinationProvider';
import { VehicleConfigProvider } from '@/providers/VehicleConfigProvider';
import { VehicleVisionProvider } from '@/providers/VehicleVisionProvider';
import { VehicleLoggingProvider } from '@/providers/VehicleLoggingProvider';
import GlobalVehicleShortcuts from '@/components/system/GlobalVehicleShortcuts';

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
      <body className='h-screen overflow-hidden'>
        <ThemeProvider
          attribute='class'
          defaultTheme='system'
          enableSystem
          disableTransitionOnChange
        >
          <NextAuthSessionProvider>
            <div className='h-full w-full flex flex-col md:px-4 lg:px-8'>
              <DestinationProvider>
                <VehicleConfigProvider>
                  <VehicleVisionProvider>
                    <VehicleLoggingProvider>
                      <GlobalVehicleShortcuts />
                      {children}
                    </VehicleLoggingProvider>
                  </VehicleVisionProvider>
                </VehicleConfigProvider>
              </DestinationProvider>
            </div>
          </NextAuthSessionProvider>
        </ThemeProvider>
      </body>
    </html>
  );
}
