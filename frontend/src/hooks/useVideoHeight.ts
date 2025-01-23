import { useState, useEffect } from 'react';

const useVideoHeight = () => {
  const [videoHeight, setVideoHeight] = useState<number>(
    window.innerHeight - 160
  );

  useEffect(() => {
    const updateHeight = () => {
      setVideoHeight(window.innerHeight - 152);
    };

    // Atualiza a altura ao redimensionar a janela
    window.addEventListener('resize', updateHeight);

    // Limpa o listener ao desmontar
    return () => {
      window.removeEventListener('resize', updateHeight);
    };
  }, []);

  return videoHeight;
};

export default useVideoHeight;
