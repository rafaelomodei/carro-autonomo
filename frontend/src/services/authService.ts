import { auth } from '@/config/firebaseConfig';
import { signInWithEmailAndPassword } from 'firebase/auth';

export const verifyUser = async (email: string, password: string) => {
  try {
    const userCredential = await signInWithEmailAndPassword(
      auth,
      email,
      password
    );
    const user = userCredential.user;

    return {
      id: user.uid,
      name: user.displayName || 'Usuário',
      email: user.email,
    };
  } catch (error) {
    console.error('Erro ao autenticar usuário:', error);
    return null;
  }
};
