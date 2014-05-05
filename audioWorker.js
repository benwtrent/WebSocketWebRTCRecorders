var ws;

this.onmessage = function (e) {
  switch (e.data.command) {
    case 'init':
      init(e.data.config);
      break;
    case 'record':
      record(e.data.samples);
      break;
  }
};

function init(config) {
  ws = new WebSocket(config.uri, config.protocol);
}

function record(samples) {
  ws.send(samples);
}