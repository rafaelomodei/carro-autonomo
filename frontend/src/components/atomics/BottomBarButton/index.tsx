import { Button } from '@/components/ui/button';
import { LucideIcon } from 'lucide-react'; // Tipo de ícones Lucide

interface BottomBarButtonProps {
  label: string;
  Icon: LucideIcon;
  isActive: boolean;
  onClick: () => void;
}

const BottomBarButton = ({
  label,
  Icon,
  isActive,
  onClick,
}: BottomBarButtonProps) => {
  return (
    <Button
      variant={isActive ? 'secondary' : 'ghost'}
      className={`flex items-center space-x-2 px-4 py-2 rounded-full transition-colors duration-200`}
      onClick={onClick}
    >
      <Icon className='w-5 h-5' />
      <span>{label}</span>
    </Button>
  );
};

export default BottomBarButton;
