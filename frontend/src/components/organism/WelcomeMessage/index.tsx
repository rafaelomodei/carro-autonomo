import { useEffect, useState } from 'react';
import { WelcomeMessageProps } from './interface';

const WelcomeMessage: React.FC<WelcomeMessageProps> = ({ name }) => {
  const messages = [
    { part1: `Olá, ${name}`, part2: 'Pronto para a próxima jornada?' },
    { part1: `Olá, ${name}`, part2: 'Tudo pronto para seguir adiante?' },
    {
      part1: `Bem-vindo, ${name}`,
      part2: 'Vamos começar sua próxima jornada?',
    },
    { part1: `Olá, ${name}`, part2: 'Pronto para explorar novos caminhos?' },
    { part1: `Bem-vindo de volta, ${name}`, part2: 'Vamos em frente?' },
    { part1: `Olá, ${name}`, part2: 'Preparado para seguir em segurança?' },
    { part1: `Bom te ver, ${name}`, part2: 'A estrada espera por você.' },
    { part1: `Olá, ${name}`, part2: 'Tudo pronto para começar a viagem?' },
    {
      part1: `Olá, ${name}`,
      part2: 'Pronto para uma nova experiência de direção?',
    },
  ];

  const [message, setMessage] = useState<{ part1: string; part2: string }>({
    part1: '',
    part2: '',
  });

  useEffect(() => {
    const randomMessage = messages[Math.floor(Math.random() * messages.length)];
    setMessage(randomMessage);
  }, [name]);

  return (
    <div className='text-center'>
      <h1 className='text-5xl font-bold'>{message.part1}</h1>
      <h2 className='text-4xl font-bold mt-2'>{message.part2}</h2>
    </div>
  );
};

export default WelcomeMessage;
