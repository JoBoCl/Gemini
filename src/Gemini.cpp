#include <algorithm>
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

class OscillatorState {
  float phase = -1.f;  // in [-1, 1
  float pitch = 0.f;   // Unbounded, usually between [-10, 10]
  const float baseFrequency;
  float frequency;     // baseFrequency * 2 ^ pitch;
  bool cycle = false;  // Used to determine the current cycle for the sub-pulse.

 public:
  OscillatorState(float startingFrequency)
      : baseFrequency(startingFrequency), frequency(startingFrequency) {}

  // For hard sync
  void resetPhase() {
    phase = -1.f;
    cycle = false;
  }

  // Returns true if reset occurs.
  bool updatePhase(float sampleTime) {
    phase += frequency * sampleTime;
    if (phase >= 1.f) {
      phase -= 2.f;
      this->cycle = !this->cycle;
      return true;
    }
    return false;
  }

  void updatePitch(float pitch) {
    if (this->pitch == pitch) {
      return;
    }
    this->pitch = pitch;
    this->frequency = baseFrequency * std::pow(2.f, pitch);
  }

  float triangle() {  // phase \in [-1, 1)
    float triangle = this->phase + 1.f;
    if (triangle > 1.f) {
      triangle = 1.f - (triangle - 1.f);  // triangle \in [0, 1)
    }
    triangle *= 2.f;        // triangle \in [0, 2)
    return triangle - 1.f;  // result in [-1, 1)
  }

  float ramp() {  // phase \in [-1, 1)
    return -this->phase;
  }

  // Shift the phase to make it match gemini.wntr.dev diagrams.
  float sub() {  // phase \in [-1, 1)
    float sub = this->phase + (cycle ? 0.5f : 0.f);
    return 0 < sub && sub <= 1 ? -1.f : 1.f;
  }

  // phase \in [-1, 1), duty in [0, 1), offset \in [0, 1]
  float pulse(float duty, float offset = 0.f) {
    float normal = (phase + 1.f) / 2.f;
    return normal > duty ? -1.f : 1.f;
  }
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
  OscillatorState castor = OscillatorState(rack::dsp::FREQ_C4);
  OscillatorState pollux = OscillatorState(rack::dsp::FREQ_C4);
  OscillatorState lfo = OscillatorState(2.f);

  Gemini() {
    config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    configParam(CASTOR_PITCH_PARAM, -1.f, 1.f, 0.f, "Castor pitch");
    configParam(POLLUX_PITCH_PARAM, -1.f, 1.f, 0.f, "Pollux pitch");
    configParam(LFO_PARAM, 0.f, 1.f, 0.f, "LFO");
    configParam(CASTOR_DUTY_PARAM, 0.f, 1.f, 0.f, "Castor duty");
    configParam(POLLUX_DUTY_PARAM, 0.f, 1.f, 0.f, "Pollux duty");
    configParam(CROSSFADE_PARAM, 0.f, 1.f, 0.5f, "Crossfade");
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

  json_t* dataToJson() override {
    json_t* rootJ = json_object();
    for (size_t paramInt = CASTOR_PITCH_PARAM; paramInt != PARAMS_LEN;
         ++paramInt) {
      ParamId param = static_cast<ParamId>(paramInt);
      for (size_t modeInt = 0; modeInt < 4; ++modeInt) {
        Mode mode = static_cast<Mode>(modeInt);
        std::string name;
        sprintf(&name[0], "%ld/%ld/%d", paramInt, modeInt, true);
        json_t* val = json_real(
            static_cast<double>(this->getParamRef(true, mode, param)));
        json_object_set_new(rootJ, name.c_str(), val);
        sprintf(&name[0], "%ld/%ld/%d", paramInt, modeInt, false);
        val = json_real(
            static_cast<double>(this->getParamRef(false, mode, param)));
        json_object_set_new(rootJ, name.c_str(), val);
      }
    }
    return rootJ;
  }

  void dataFromJson(json_t* rootJ) override {
    const char* key;
    json_t* value;
    json_object_foreach(rootJ, key, value) {
      int32_t paramInt, modeInt, altModeInt;
      sscanf(key, "%d/%d/%d", &paramInt, &modeInt, &altModeInt);
      this->getParamRef(
          static_cast<bool>(altModeInt), static_cast<Mode>(modeInt),
          static_cast<ParamId>(paramInt)) = json_number_value(value);
    }
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
            return altMode ? lfoPwmCastorPulseWidthCentre : castorDutyParam;
          case LFO_FM:
            return castorDutyParam;
          case HARD_SYNC:
            return castorDutyParam;
        }
      case POLLUX_DUTY_PARAM:
        switch (lfoMode) {
          case CHORUS:
            return polluxDutyParam;
          case LFO_PWM:
            return altMode ? lfoPwmPolluxPulseWidthCentre : polluxDutyParam;
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

  inline Mode getMode() { return this->mode; }

  void updateParams() {
    bool nowAltMode = params[ALT_MODE_BUTTON_PARAM].getValue() == 1.f;
    Mode nowMode = static_cast<Mode>(
        static_cast<int32_t>(params[BUTTON_PARAM].getValue()));

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

  /*
   * Main logic thread
   *
   * Gemini is made up of two oscillators - Castor and Pollux. Castor can be
   * thought of as the main driving oscillator, with its frequency influencing
   * the frequency of Pollux (unless a separate CV input is provided).
   *
   * Each mode has a different behaviour:
   *   * Chorus - subtle frequency modulation of Pollux only.
   *   * LFO PWM - pulse width influenced by LFO output.
   *   * LFO FM - frequency influenced by LFO output.
   *   * Hard sync - Pollux gets reset every time Castor does. Pollux's
   *       frequency becomes a multiple of Castor's.
   *
   * Update user-driven parameters every 128 ticks - this saves some resources
   * and should be imperceptible to the vast majority of users.
   *
   * Update the pitch of the oscillators each tick, as they are externally
   * driven and can change more frequently. The oscillators need updating every
   * tick to ensure a smooth output.
   *
   * When the oscillators' frequency changes, keep the same phase to ensure a
   * smooth transition between different frequencies.
   */
  void process(const ProcessArgs& args) override {
    if (args.frame % 128 == 0) {
      updateParams();
    }

    lfo.updatePitch(this->getLfoCv());
    lfo.updatePhase(args.sampleTime);

    castor.updatePitch(this->getCastorPitchCv());
    bool castorReset = castor.updatePhase(args.sampleTime);

    pollux.updatePitch(this->getPolluxPitchCv());
    if (mode == HARD_SYNC && castorReset) {
      pollux.resetPhase();
    } else {
      pollux.updatePhase(args.sampleTime);
    }

    Signals castorSignals = this->getSignals(castor, this->getCastorDutyCycle(),
                                             this->getCastorPulseOffset());
    Signals castorMix = this->getCastorMix();

    // Audio signals are typically +/-5V
    // https://vcvrack.com/manual/VoltageStandards
    float castorOut = this->getOutput(castorSignals, castorMix);
    outputs[CASTOR_MIX_OUTPUT].setVoltage(castorOut);

    // Pollux's behaviour generally depends on the current mode.
    Signals polluxSignals = this->getSignals(pollux, this->getPolluxDutyCycle(),
                                             this->getPolluxPulseOffset());
    Signals polluxMix = this->getPolluxMix();
    float polluxOut = this->getOutput(polluxSignals, polluxMix);
    outputs[POLLUX_MIX_OUTPUT].setVoltage(polluxOut);

    outputs[MIX_OUTPUT].setVoltage(
        this->getMix(castorOut, polluxOut, this->getParamRef(CROSSFADE_PARAM)));
  }

 private:
  inline float getCastorPulseOffset() {
    return this->getPulseOffset(/*isCastor=*/true);
  }

  inline float getPolluxPulseOffset() {
    return this->getPulseOffset(/*isCastor=*/false);
  }

  inline float getPulseOffset(bool isCastor) {
    return this->getMode() == LFO_PWM
               ? this->getParamRef(
                     true, LFO_PWM,
                     isCastor ? CASTOR_DUTY_PARAM : POLLUX_DUTY_PARAM)
               : 0.f;
  }

  float getOutput(Signals& wave, Signals& amplitude) {
    return 5.f *
           (wave.ramp * amplitude.ramp + wave.pulse * amplitude.pulse +
            wave.sub * amplitude.sub) /
           3.f;
  }

  inline float getMix(float castor, float pollux, float mix) {
    return rack::simd::crossfade(castor, pollux, mix);
  }

  Signals getSignals(OscillatorState& osc, float duty, float offset) {
    return {
        osc.ramp(),
        osc.pulse(duty, offset),
        osc.sub(),
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

  // Returns a value in [0, 1.f].
  float getDutyCycle(InputId input, ParamId param) {
    float baseDutyCycle =
        this->getParamRef(param) + inputs[input].getNormalVoltage(0.f) / 5.f;
    if (this->getMode() == LFO_PWM) {
      // LFO Value \in [-1, 1]
      baseDutyCycle += this->getLfoValue();
    }

    return rack::math::clamp(baseDutyCycle, -1.f, 1.f);
  }

  float getCastorDutyCycle() {
    return this->getDutyCycle(CASTOR_DUTY_INPUT, CASTOR_DUTY_PARAM);
  }

  float getPolluxDutyCycle() {
    return this->getDutyCycle(POLLUX_DUTY_INPUT, POLLUX_DUTY_PARAM);
  }

  float getCastorPitchCv() {
    float basePitch = this->getCastorPitchCvBase();
    if (getMode() != LFO_FM) {
      return basePitch;
    }
    float lfoValue = this->getLfoValue();
    return basePitch + lfoValue;
  }

  float getCastorPitchCvBase() {
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
    float value = this->lfo.triangle();  // in [-1, 1)
    if (this->getMode() == CHORUS || this->getMode() == HARD_SYNC) {
      value *= std::log(getParamRef(false, this->getMode(), LFO_PARAM) + 1);
    }
    return value;
  }

  // Current value determining the frequency of the LFO
  float getLfoCv() {
    // Param \in [-1, 1]
    float param = this->getParamRef(this->getMode() == CHORUS || altMode,
                                    this->getMode(), LFO_PARAM);
    return 5.f * (param + 1.f);
  }

  float getPolluxPitchCv() {
    float polluxBasePitchCv = this->getPolluxBasePitchCv();
    if (this->getMode() == HARD_SYNC) {
      auto basePitchCv =
          inputs[POLLUX_PITCH_PARAM].isConnected()
              ? (rack::math::clamp(inputs[POLLUX_PITCH_PARAM].getVoltage(),
                                   -6.f, 6.f))
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
