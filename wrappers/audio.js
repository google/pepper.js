(function() {

  var createAudioContext = function() {
    var AudioContext = window.AudioContext || window.webkitAudioContext;
    return new AudioContext();
  }

  // The Web Audio API currently does not allow user-specified sample rates.
  var supportedSampleRate = function() {
    return createAudioContext().sampleRate;
  }

  var AudioConfig_CreateStereo16Bit = function(instance, sample_rate, sample_frame_count) {
    if (sample_rate !== supportedSampleRate()) {
      return 0;
    }
    // PP_AUDIOMINSAMPLEFRAMECOUNT = 64
    // PP_AUDIOMAXSAMPLEFRAMECOUNT = 32768
    if (sample_frame_count < 64 || sample_frame_count > 32768) {
      return 0;
    }
    return resources.register("audio_config", {
      sample_rate: sample_rate,
      sample_frame_count: sample_frame_count
    });
  };

  var AudioConfig_RecommendSampleFrameCount = function(instance, sample_rate, requested_sample_frame_count) {
    // TODO(ncbray): be smarter?
    return 2048;
  };

  var AudioConfig_IsAudioConfig = function(resource) {
    return resources.is(resource, "audio_config");
  };

  var AudioConfig_GetSampleRate = function(config) {
    var c = resources.resolve(config);
    if (c === undefined) {
      return 0;
    }
    return c.sample_rate;
  };

  var AudioConfig_GetSampleFrameCount = function(config) {
    var c = resources.resolve(config);
    if (c === undefined) {
      return 0;
    }
    return c.sample_frame_count;
  };

  var AudioConfig_RecommendSampleRate = function(instance) {
    return supportedSampleRate();
  };

  registerInterface("PPB_AudioConfig;1.1", [
    AudioConfig_CreateStereo16Bit,
    AudioConfig_RecommendSampleFrameCount,
    AudioConfig_IsAudioConfig,
    AudioConfig_GetSampleRate,
    AudioConfig_GetSampleFrameCount,
    AudioConfig_RecommendSampleRate,
  ]);

  registerInterface("PPB_AudioConfig;1.0", [
    AudioConfig_CreateStereo16Bit,
    AudioConfig_RecommendSampleFrameCount,
    AudioConfig_IsAudioConfig,
    AudioConfig_GetSampleRate,
    AudioConfig_GetSampleFrameCount,
  ]);

  var SetPlaybackState = function(audio, state) {
    if (audio.playing === state) {
      return;
    }
    if (state) {
      audio.processor.connect(audio.context.destination);
      audio.playing = true;
    } else {
      audio.processor.disconnect();
      audio.playing = false;
    }
  };

  var Audio_Create = function(instance, config, audio_callback, user_data) {
    var config_js = resources.resolve(config);
    // Assumes 16-bit stereo.
    var buffer_bytes = config_js.sample_frame_count * 2 * 2;
    var buffer =  _malloc(buffer_bytes);

    var context = createAudioContext();
    // Note requires power-of-two buffer size.
    var processor = context.createScriptProcessor(config_js.sample_frame_count, 0, 2);
    processor.onaudioprocess = function (e) {
      Runtime.dynCall('viii', audio_callback, [buffer, buffer_bytes, user_data]);
      var l = e.outputBuffer.getChannelData(0);
      var r = e.outputBuffer.getChannelData(1);
      var base = buffer>>1;
      var offset = 0;
      var scale = 1 / ((1<<15)-1);
      for (var i = 0; i < e.outputBuffer.length; i++) {
        l[i] = HEAP16[base + offset] * scale;
        offset += 1;
        r[i] = HEAP16[base + offset] * scale;
        offset += 1;
      }
    };

    // TODO(ncbray): capture and ref/unref the config?
    // TODO(ncbray): destructor?
    return resources.register("audio", {
      context: context,
      processor: processor,
      playing: false,
      audio_callback: audio_callback,
      user_data: user_data,
      // Assumes 16-bit stereo.
      buffer: buffer
    });
  };

  var Audio_IsAudio = function(resource) {
    return resources.is(resource, "audio");
  };

  var Audio_GetCurrentConfig = function(audio) {
    throw "Audio_GetCurrentConfig not implemented";
  };

  var Audio_StartPlayback = function(audio) {
    SetPlaybackState(resources.resolve(audio), true);
  };

  var Audio_StopPlayback = function(audio) {
    SetPlaybackState(resources.resolve(audio), false);
  };

  registerInterface("PPB_Audio;1.0", [
    Audio_Create,
    Audio_IsAudio,
    Audio_GetCurrentConfig,
    Audio_StartPlayback,
    Audio_StopPlayback,
  ]);

})();
