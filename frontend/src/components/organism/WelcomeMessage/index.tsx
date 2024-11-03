import { useEffect, useState } from 'react';
import { WelcomeMessageProps } from './interface';
import { getMessages } from './constants';

const WelcomeMessage: React.FC<WelcomeMessageProps> = ({ name }) => {
  const [message, setMessage] = useState<{ part1: string; part2: string }>({
    part1: '',
    part2: '',
  });

  useEffect(() => {
    const messages = getMessages(name);
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
