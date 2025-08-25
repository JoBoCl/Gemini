#include <cassert>
#include <cmath>
#include <map>
#include <vector>

#include "plugin.hpp"

// TOOD - maybe this could be one of those fancy simd things?
struct Signals {
  float ramp;
  float pulse;
  float sub;
};

struct Gemini : Module {
  enum ParamId {
    CASTOR_PITCH_PARAM,
    POLLUX_PITCH_PARAM,
    LFO_PARAM,
    CASTOR_DUTY_PARAM,
    POLLUX_DUTY_PARAM,
    CROSSFADE_PARAM,
    CASTOR_RAMP_LEVEL_PARAM,
    CASTOR_PULSE_LEVEL_PARAM,
    POLLUX_PULSE_LEVEL_PARAM,
    BUTTON_PARAM,
    CASTOR_SUB_LEVEL_PARAM,
    POLLUX_SUB_LEVEL_PARAM,
    POLLUX_RAMP_LEVEL_PARAM,
    ALT_MODE_BUTTON_PARAM,
    PARAMS_LEN
  };
  enum InputId {
    CASTOR_DUTY_INPUT,
    POLLUX_DUTY_INPUT,
    CASTOR_PITCH_INPUT,
    POLLUX_PITCH_INPUT,
    INPUTS_LEN
  };
  enum OutputId {
    CASTOR_MIX_OUTPUT,
    MIX_OUTPUT,
    POLLUX_MIX_OUTPUT,
    OUTPUTS_LEN
  };
  enum LightId { LIGHTS_LEN };

  enum Mode {
    CHORUS,
    LFO_PWM,
    LFO_FM,
    HARD_SYNC,
  };

  Mode getMode() {
    return static_cast<Mode>(
        static_cast<int32_t>(params[BUTTON_PARAM].getValue()));
  }

  // Param Values - updated by user, can be slightly stale.
  float castorPitchParam, castorDutyParam, castorRampLevelParam,
      castorPulseLevelParam, castorSubParam;
  float polluxPitchParam, polluxDutyParam, polluxRampLevelParam,
      polluxPulseLevelParam, polluxSubParam;
  float lfoParam;
  float buttonParam;
  float altModeParam;
  float crossfadeParam;

  Mode mode = CHORUS;
  bool altMode = false;

  float altModeLfoCv = 0.f;
  float lfoAmplitudeValue = 0.f;
  float lfoChorusFreqCv = 0.f;
  float lfoPwmFreqCv = 0.f;
  float lfoFmFreqCv = 0.f;
  float lfoHardSyncFreqCv = 0.f;
  float lfoPwmCastorPulseWidthCentre = 0.f;
  float lfoPwmPolluxPulseWidthCentre = 0.f;
  float lfoFmCastorPulseWidth = 0.5f;
  float lfoFmPolluxPulseWidth = 0.5f;
  float polluxPitchMultiplier = -1.f;

  float paramsLen = -3.14f;

  // State
  float castorPhase = 0.f;
  float polluxPhase = 0.f;
  float castorSubPhase = 0.f;
  float polluxSubPhase = 0.f;
  float lfoPhase = 0.f;

  Gemini() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configParam(CASTOR_PITCH_PARAM, -1.f, 1.f, 0.f, "Castor pitch");
    configParam(POLLUX_PITCH_PARAM, -1.f, 1.f, 0.f, "Pollux pitch");
    configParam(LFO_PARAM, 0.f, 1.f, 0.f, "LFO");
    configParam(CASTOR_DUTY_PARAM, 0.f, 1.f, 0.f, "Castor duty");
    configParam(POLLUX_DUTY_PARAM, 0.f, 1.f, 0.f, "Pollux duty");
    configParam(CROSSFADE_PARAM, -1.f, 1.f, 0.f, "Crossfade");
    configParam(CASTOR_RAMP_LEVEL_PARAM, 0.f, 1.f, 0.f, "Castor ramp level");
    configParam(CASTOR_PULSE_LEVEL_PARAM, 0.f, 1.f, 0.f, "Castor pulse level");
    configParam(POLLUX_PULSE_LEVEL_PARAM, 0.f, 1.f, 0.f, "Pollux pulse level");
    configParam(BUTTON_PARAM, 0.f, 3.f, 0.f, "Mode switch");
    configParam(CASTOR_SUB_LEVEL_PARAM, 0.f, 1.f, 0.f, "Castor sub level");
    configParam(POLLUX_SUB_LEVEL_PARAM, 0.f, 1.f, 0.f, "Pollux sub level");
    configParam(POLLUX_RAMP_LEVEL_PARAM, 0.f, 1.f, 0.f, "Pollux ramp level");
    configParam(ALT_MODE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Alt Mode switch");

    configInput(CASTOR_DUTY_INPUT, "Castor duty");
    configInput(POLLUX_DUTY_INPUT, "Pollux duty");
    configInput(CASTOR_PITCH_INPUT, "Castor pitch");
    configInput(POLLUX_PITCH_INPUT, "Pollux pitch");

    configOutput(CASTOR_MIX_OUTPUT, "Castor");
    configOutput(MIX_OUTPUT, "Mix");
    configOutput(POLLUX_MIX_OUTPUT, "Pollux");
  }

  inline float& getParamRef(ParamId param) {
    return this->getParamRef(this->altMode, this->mode, param);
  }

  float& getParamRef(bool altMode, Mode lfoMode, ParamId param) {
    switch (param) {
      case CASTOR_PITCH_PARAM:
        return castorPitchParam;
      case POLLUX_PITCH_PARAM:
        return lfoMode == HARD_SYNC ? polluxPitchMultiplier : polluxPitchParam;
      case CASTOR_RAMP_LEVEL_PARAM:
        return castorRampLevelParam;
      case CASTOR_PULSE_LEVEL_PARAM:
        return castorPulseLevelParam;
      case POLLUX_PULSE_LEVEL_PARAM:
        return polluxPulseLevelParam;
      case BUTTON_PARAM:
        return buttonParam;
      case CASTOR_SUB_LEVEL_PARAM:
        return castorSubParam;
      case POLLUX_SUB_LEVEL_PARAM:
        return polluxSubParam;
      case POLLUX_RAMP_LEVEL_PARAM:
        return polluxRampLevelParam;
      case ALT_MODE_BUTTON_PARAM:
        return altModeParam;
      case CROSSFADE_PARAM:
        return crossfadeParam;
      case CASTOR_DUTY_PARAM:
        switch (lfoMode) {
          case CHORUS:
            return castorDutyParam;
          case LFO_PWM:
            return lfoPwmFreqCv;
          case LFO_FM:
            return lfoFmFreqCv;
          case HARD_SYNC:
            return lfoHardSyncFreqCv;
        }
      case POLLUX_DUTY_PARAM:
        switch (lfoMode) {
          case CHORUS:
            return polluxDutyParam;
          case LFO_PWM:
            return polluxDutyParam;
          case LFO_FM:
            return polluxDutyParam;
          case HARD_SYNC:
            return polluxDutyParam;
        }
      case LFO_PARAM:
        switch (lfoMode) {
          case CHORUS:
            return altMode ? lfoAmplitudeValue : lfoChorusFreqCv;
          case LFO_PWM:
            return lfoPwmFreqCv;
          case LFO_FM:
            return lfoFmFreqCv;
          case HARD_SYNC:
            return lfoHardSyncFreqCv;
        }
      case PARAMS_LEN:
        return paramsLen;
    }
  }

  void updateParams() {
    bool nowAltMode = params[ALT_MODE_BUTTON_PARAM].getValue() == 1.f;
    Mode nowMode = this->getMode();

    if (nowAltMode != altMode || nowMode != mode) {
      // Assume that users cannot click between the alt mode button and the
      // UI controls within 2ms. Update the controls with the alt-mode (or
      // standard) mode values (so that they know what they're altering).
      this->altMode = nowAltMode;
      this->mode = nowMode;
      for (int paramInt = CASTOR_PITCH_PARAM; paramInt != PARAMS_LEN;
           ++paramInt) {
        ParamId p = static_cast<ParamId>(paramInt);
        if (p == BUTTON_PARAM || p == ALT_MODE_BUTTON_PARAM) {
          continue;
        }
        float& ref = this->getParamRef(nowAltMode, nowMode, p);
        params[p].setValue(ref);
      }
    } else {
      for (int paramInt = CASTOR_PITCH_PARAM; paramInt != PARAMS_LEN;
           ++paramInt) {
        ParamId p = static_cast<ParamId>(paramInt);
        float& ref = this->getParamRef(nowAltMode, nowMode, p);
        ref = params[p].getValue();
      }
    }
  }

  void process(const ProcessArgs& args) override {
    if (args.frame % 128 == 0) {
      updateParams();
    }

    float lfoFreq = getLfoFreq(this->getLfoCv());
    lfoPhase += lfoFreq * args.sampleTime;
    if (lfoPhase >= 1.f) {
      lfoPhase -= 2.f;
    }

    bool castorReset = false;
    {
      float castorFreq = getPitchFreq(this->getCastorPitchCv());
      // Accumulate the phase
      float castorUpdate = castorFreq * args.sampleTime;
      castorPhase += castorUpdate;
      castorSubPhase += castorUpdate / 2.f;
      if (castorPhase >= 1.f) {
        castorPhase -= 2.f;
        castorReset = true;
      }
      if (castorSubPhase >= 1.f) {
        castorSubPhase -= 2.f;
      }
    }

    Signals castor = this->getSignals(castorPhase, this->getCastorDutyCycle(),
                                      castorSubPhase);
    Signals castorMix = this->getCastorMix();

    // Audio signals are typically +/-5V
    // https://vcvrack.com/manual/VoltageStandards
    float castorOut = 5.f * this->getOutput(castor, castorMix);
    outputs[CASTOR_MIX_OUTPUT].setVoltage(castorOut);

    // Pollux's behaviour generally depends on the current mode.
    float polluxFreq = getPitchFreq(this->getPolluxPitchCv());
    if (mode == HARD_SYNC && castorReset) {
      polluxPhase = -1.f;
      polluxSubPhase = -1.f;
    } else {
      float polluxUpdate = polluxFreq * args.sampleTime;
      polluxPhase += polluxUpdate;
      polluxSubPhase += polluxUpdate / 2.f;
      if (polluxPhase >= 1.f) {
        polluxPhase -= 2.f;
      }
      if (polluxSubPhase >= 1.f) {
        polluxSubPhase -= 2.f;
      }
    }
    Signals pollux = this->getSignals(polluxPhase, this->getPolluxDutyCycle(),
                                      polluxSubPhase);
    Signals polluxMix = this->getPolluxMix();
    float polluxOut = 5.f * this->getOutput(pollux, polluxMix);
    outputs[POLLUX_MIX_OUTPUT].setVoltage(polluxOut);

    outputs[MIX_OUTPUT].setVoltage(
        this->getMix(castorOut, polluxOut, params[CROSSFADE_PARAM].getValue()));
  }

 private:
  float getOutput(Signals wave, Signals amplitude) {
    return (wave.ramp * amplitude.ramp + wave.pulse * amplitude.pulse +
            wave.sub * amplitude.sub) /
           3.f;
  }

  // castor \in [-5, 5), pollux \in [-5, 5), mix \in [-1, 1]
  // mix == -1 => only castor
  // mix ==  0 => both heard equally
  // mix ==  1 => only pollux
  float getMix(float castor, float pollux, float mix) {
    float pollux_vol = (1 + mix) / 2.f;
    float castor_vol = 1 - pollux_vol;

    return castor * castor_vol + pollux * pollux_vol;
  }

  float triangle(float phase) {  // phase \in [-1, 1)
    phase += 1.f;                // phase \in [0, 2)
    if (phase > 1.f) {
      phase = 1.f - (phase - 1.f);  // phase \in [0, 1)
    }
    phase *= 2.f;        // phase \in [0, 2)
    return phase - 1.f;  // result in [-1, 1)
  }

  float ramp(float phase) {  // phase \in [-1, 1)
    return -phase;
  }

  // Shift the phase to make it match gemini.wntr.dev diagrams.
  float sub(float phase) {  // phase \in [-1, 1)
    return 0 < phase + 0.5f && phase + 0.5f <= 1 ? -1.f : 1.f;
  }

  // phase \in [-1, 1), duty in [0, 1)
  float pulse(float phase, float duty, float offset = 0.f) {
    float normal = (phase + 1.f) / 2.f;
    return normal > duty ? -1.f : 1.f;
  }

  Signals getSignals(float phase, float duty, float subPhase) {
    return {
        this->ramp(phase),
        this->pulse(phase, duty),
        this->sub(subPhase),
    };
  }

  Signals getCastorMix() {
    return this->getMix(CASTOR_RAMP_LEVEL_PARAM, CASTOR_PULSE_LEVEL_PARAM,
                        CASTOR_SUB_LEVEL_PARAM);
  }

  Signals getPolluxMix() {
    return this->getMix(POLLUX_RAMP_LEVEL_PARAM, POLLUX_PULSE_LEVEL_PARAM,
                        POLLUX_SUB_LEVEL_PARAM);
  }

  Signals getMix(ParamId ramp, ParamId pulse, ParamId sub) {
    return {
        this->getParamRef(ramp),
        this->getParamRef(pulse),
        this->getParamRef(sub),
    };
  }

  float getDutyCycle(InputId input, ParamId param) {
    if (inputs[input].isConnected()) {
      return rack::math::clamp(inputs[input].getVoltage(), 0.f, 10.f) / 10.f;
    } else {
      return params[param].getValue();
    }
  }

  float getCastorDutyCycle() {
    return this->getDutyCycle(CASTOR_DUTY_INPUT, CASTOR_DUTY_PARAM);
  }

  float getPolluxDutyCycle() {
    return this->getDutyCycle(POLLUX_DUTY_INPUT, POLLUX_DUTY_PARAM);
  }

  float getSubCv(float cv) { return cv - 1.f; }

  float getPitchFreq(float cv) { return dsp::FREQ_C4 * std::pow(2.f, cv + 1); }

  float getLfoFreq(float cv) {
    // From https://vcvrack.com/manual/VoltageStandards
    // Low-frequency oscillators and clock generators should use 120 BPM
    return 2.f * std::pow(2.f, cv);
  }

  float getCastorPitchCv() {
    if (inputs[CASTOR_PITCH_INPUT].isConnected()) {
      // Return Castor pitch with a the offset from the knob.
      return inputs[CASTOR_PITCH_INPUT].getVoltage() +
             params[CASTOR_PITCH_PARAM].getValue();
    } else {
      // Quantize the knob output.
      // When there's no input to Castor, it has a +/- 3 Oct swing.
      float pitchCv = getParamRef(altMode, mode, CASTOR_PITCH_PARAM) * 3.f;
      float remainder = std::remainderf(pitchCv, 1.f / 12.f);
      return pitchCv - remainder;
    }
  }

  float getLfoValue() {
    // We need to attenuate it based on the LFO_PARAM
    float value = this->triangle(this->lfoPhase);  // in [-1, 1)
    if (this->getMode() == CHORUS || this->getMode() == HARD_SYNC) {
      value *= std::log(getParamRef(false, this->getMode(), LFO_PARAM) + 1);
    }
    return value;
  }

  // Current value determining the frequency of the LFO
  float getLfoCv() {
    float param = this->getParamRef(this->mode == CHORUS || altMode, this->mode, LFO_PARAM);
    return 5.f * (param + 1.f);
  }

  float getPolluxPitchCv() {
    float polluxBasePitchCv = this->getPolluxBasePitchCv();
    if (this->getMode() == HARD_SYNC) {
      auto basePitchCv =
          inputs[POLLUX_PITCH_PARAM].isConnected()
              ? (std::clamp(inputs[POLLUX_PITCH_PARAM].getVoltage(), -6.f, 6.f))
              : this->getCastorPitchCv();
      return basePitchCv + ((1.f + getParamRef(POLLUX_PITCH_PARAM)) * 1.5f);
    }
    if (this->getMode() == CHORUS) {
      return polluxBasePitchCv + this->getLfoValue();
    }
    // switch (this->getMode()) { case CHORUS: case HARD_SYNC: }
    return polluxBasePitchCv;
  }

  float getPolluxBasePitchCv() {
    float mainPitchCv = inputs[POLLUX_PITCH_INPUT].isConnected()
                            ? inputs[POLLUX_PITCH_INPUT].getVoltage()
                            : this->getCastorPitchCv();

    return mainPitchCv + params[POLLUX_PITCH_PARAM].getValue();
  }
};

struct GeminiWidget : ModuleWidget {
  GeminiWidget(Gemini* module) {
    setModule(module);
    setPanel(createPanel(asset::plugin(pluginInstance, "res/Gemini.svg")));

    addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
    addChild(
        createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild(createWidget<ScrewSilver>(
        Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild(createWidget<ScrewSilver>(Vec(
        box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    addParam(createParamCentered<RoundHugeBlackKnob>(
        mm2px(Vec(16.48, 21.259)), module, Gemini::CASTOR_PITCH_PARAM));
    addParam(createParamCentered<RoundHugeBlackKnob>(
        mm2px(Vec(54.08, 21.259)), module, Gemini::POLLUX_PITCH_PARAM));
    addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(35.28, 42.455)),
                                                    module, Gemini::LFO_PARAM));
    addParam(createParamCentered<RoundBigBlackKnob>(
        mm2px(Vec(12.331, 52.077)), module, Gemini::CASTOR_DUTY_PARAM));
    addParam(createParamCentered<RoundBigBlackKnob>(
        mm2px(Vec(58.198, 52.077)), module, Gemini::POLLUX_DUTY_PARAM));
    addParam(createParamCentered<RoundBigBlackKnob>(
        mm2px(Vec(35.28, 64.716)), module, Gemini::CROSSFADE_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(7.854, 80.97)), module, Gemini::CASTOR_RAMP_LEVEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(21.536, 80.97)), module, Gemini::CASTOR_PULSE_LEVEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(62.447, 80.97)), module, Gemini::POLLUX_RAMP_LEVEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(21.536, 95.994)), module, Gemini::CASTOR_SUB_LEVEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(48.834, 95.994)), module, Gemini::POLLUX_SUB_LEVEL_PARAM));
    addParam(createParamCentered<RoundBlackKnob>(
        mm2px(Vec(48.834, 80.97)), module, Gemini::POLLUX_PULSE_LEVEL_PARAM));
    addParam(createParamCentered<RoundBlackSnapKnob>(
        mm2px(Vec(35.25, 88.806)), module, Gemini::BUTTON_PARAM));
    addParam(createParamCentered<VCVLatch>(mm2px(Vec(35.25, 99.963)), module,
                                           Gemini::ALT_MODE_BUTTON_PARAM));

    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.854, 95.994)), module,
                                             Gemini::CASTOR_DUTY_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.477, 95.994)), module,
                                             Gemini::POLLUX_DUTY_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.854, 111.12)), module,
                                             Gemini::CASTOR_PITCH_INPUT));
    addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.447, 111.12)), module,
                                             Gemini::POLLUX_PITCH_INPUT));

    addOutput(createOutputCentered<PJ301MPort>(
        mm2px(Vec(21.536, 111.12)), module, Gemini::CASTOR_MIX_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.239, 111.12)),
                                               module, Gemini::MIX_OUTPUT));
    addOutput(createOutputCentered<PJ301MPort>(
        mm2px(Vec(48.761, 111.12)), module, Gemini::POLLUX_MIX_OUTPUT));
  }
};

Model* modelGemini = createModel<Gemini, GeminiWidget>("Gemini");
