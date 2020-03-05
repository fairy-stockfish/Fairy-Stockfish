(function () {
  // Message listeners

  const listeners = [];

  Module['print'] = function(line) {
    if (listeners.length === 0) console.log(line);
    else setTimeout(function() {
      for (const listener of listeners) listener(line);
    }, 1);
  };

  Module['addMessageListener'] = function(listener) {
    listeners.push(listener);
  };

  Module['removeMessageListener'] = function(listener) {
    const idx = listeners.indexOf(listener);
    if (idx >= 0) listeners.splice(idx, 1);
  };

  // Command queue

  const queue = [];
  let backoff = 1;

  function poll() {
    const command = queue.shift();
    if (!command) return;

    const tryLater = Module.ccall('uci_command', 'number', ['string'], [command]);
    if (tryLater) queue.unshift(command);
    backoff = tryLater ? (backoff * 2) : 1;
    setTimeout(poll, backoff);
  }

  Module['postMessage'] = function(command) {
    queue.push(command);
  };

  Module['postRun'] = function() {
    Module['postMessage'] = function(command) {
      queue.push(command);
      if (queue.length == 1) poll();
    };

    poll();
  };
})();
