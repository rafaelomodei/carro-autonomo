import { auth, db } from '@/config/firebaseConfig';
import { signInWithEmailAndPassword } from 'firebase/auth';
import { doc, getDoc } from 'firebase/firestore';
import NextAuth, { NextAuthOptions } from 'next-auth';
import CredentialsProvider from 'next-auth/providers/credentials';

const nextOptions: NextAuthOptions = {
  providers: [
    CredentialsProvider({
      name: 'Credenciais',
      credentials: {
        email: {
          label: 'Email',
          type: 'email',
          placeholder: 'email@exemplo.com',
        },
        password: { label: 'Senha', type: 'password' },
      },
      async authorize(credentials) {
        try {
          const userCredential = await signInWithEmailAndPassword(
            auth,
            credentials?.email ?? '',
            credentials?.password ?? ''
          );
          const user = userCredential.user;

          const userDoc = await getDoc(doc(db, 'users', user.uid));
          const userData = userDoc.exists() ? userDoc.data() : null;

          return {
            id: user.uid,
            name: userData?.displayName || user.email,
            email: user.email,
          };
        } catch (error) {
          console.error('Erro de autenticação:', error);
          return null;
        }
      },
    }),
  ],
  pages: {
    signIn: '/',
  },
  session: {
    strategy: 'jwt',
  },
};

const handler = NextAuth(nextOptions);

export { handler as GET, handler as POST };
