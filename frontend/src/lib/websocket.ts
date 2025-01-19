export const isValidWebSocketUrl = (url?: string): boolean => {
  if (!url) return false;

  const webSocketRegex = /^(ws|wss):\/\/.+$/;

  return webSocketRegex.test(url);
};
