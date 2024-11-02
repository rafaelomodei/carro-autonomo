import { usePathname, useRouter } from 'next/navigation';
import { Home, Navigation, Settings, Bug } from 'lucide-react';
import BottomBarButton from '@/components/atomics/BottomBarButton';

const BottomBar = () => {
  const router = useRouter();
  const pathname = usePathname();

  const handleNavigation = (route: string) => {
    router.push(`/${route}`);
  };

  return (
    <nav className='flex items-center justify-center space-x-8 p-4 w-full'>
      <BottomBarButton
        label='Início'
        Icon={Home}
        isActive={pathname === '/home'}
        onClick={() => handleNavigation('home')}
      />

      <BottomBarButton
        label='Navegação'
        Icon={Navigation}
        isActive={pathname === '/navigation'}
        onClick={() => handleNavigation('navigation')}
      />

      <BottomBarButton
        label='Configuração'
        Icon={Settings}
        isActive={pathname === '/settings'}
        onClick={() => handleNavigation('settings')}
      />

      <BottomBarButton
        label='Debug'
        Icon={Bug}
        isActive={pathname === '/debug'}
        onClick={() => handleNavigation('debug')}
      />
    </nav>
  );
};

export default BottomBar;
