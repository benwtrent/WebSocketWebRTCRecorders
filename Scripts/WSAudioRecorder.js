
//NOTE, Websockets in workers ONLY WORKS IN CHROME. Script will have to be modified to work in FireFox.
(function (window) {

  var WORKER_PATH = 'audioWorker.js';
  function floatTo16BitPCM(output, offset, input) {
    for (var i = 0; i < input.length; i++, offset += 2) {
      var s = Math.max(-1, Math.min(1, input[i]));
      output.setInt16(offset, s < 0 ? s * 0x8000 : s * 0x7FFF, true);
    }
  }
  /**
  * Most of this code is copied wholesale from https://github.com/mattdiamond/Recorderjs
  * This is not Stereo, on the right channel is grabbed but that is enough for me
  */
  var WSAudioRecorder = function (source, wsURL, wsProtocol) {
    var recording = false;
    var worker = new Worker(WORKER_PATH);
    worker.postMessage({
      command: 'init',
      config: {
        uri: wsURL, protocol: wsProtocol
      }
    });
    var config = {};
    var bufferLen = 4096;
    this.context = source.context;
    this.node = (this.context.createScriptProcessor ||
                 this.context.createJavaScriptNode).call(this.context,
                                                         bufferLen, 2, 2);
    this.node.onaudioprocess = function (e) {
      if (!recording) return;
      var sample = e.inputBuffer.getChannelData(0);
      //Moved the float to 16 bit translation down to codebehind
      worker.postMessage({
        command: 'record',
        samples: sample
      });
    }

    this.record = function () {
      recording = true;
    }

    this.stop = function () {
      recording = false;
    }

    this.isRecording = function () {
      return recording;
    }

    source.connect(this.node);
    this.node.connect(this.context.destination);    //this should not be necessary, but it is
  };
  window.WSAudioRecorder = WSAudioRecorder;
})(window);