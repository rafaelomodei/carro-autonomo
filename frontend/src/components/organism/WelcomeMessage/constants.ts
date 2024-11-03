const getMessages = (name: string) => [
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

export { getMessages };
