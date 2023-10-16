
Bun.serve({
  port: 3000,
  fetch(req, server) {
    if (server.upgrade(req)) {
      return;
    }
    return new Response('Upgrade failed :(', { status: 500 });
  },
  websocket: {
    message(ws, message) {
      ws.publish("general", message);
    },
    open(ws) {
        console.log("New client connected");
        ws.subscribe('general');
    },
    close(ws, code, message) {
        console.log(`Client disconnected with code: ${code} due to ${message}`)
    },
    drain(ws) {} 
  }
});


console.log('server is up at port 3000')