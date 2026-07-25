const { WebSocketServer } = require('ws');

const wss = new WebSocketServer({ port: 8080 });

console.log('[WS Server] Standard Node.js ws daemon listening on port 8080');

wss.on('connection', (ws, req) => {
  console.log('[WS Server] Client connected from ' + req.socket.remoteAddress);
  
  ws.on('message', (message, isBinary) => {
    // Echo back text or binary frame
    ws.send(message, { binary: isBinary });
  });
});
