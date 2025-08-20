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
	enum LightId {
		LIGHTS_LEN
	};

	float castorPhase = 0.f;
	float polluxPhase = 0.f;
	float castorSubPhase = 0.f;
	float polluxSubPhase = 0.f;
	float lfoPhase = 0.f;

	Gemini() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CASTOR_PITCH_PARAM, -1.f, 1.f, 0.f, "Castor pitch");
		configParam(POLLUX_PITCH_PARAM, -1.f, 1.f, 0.f, "Pollux pitch");
		configParam(LFO_PARAM, 0.f, 1.f, 0.f, "LFO rate");
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
		configInput(CASTOR_DUTY_INPUT, "Castor duty");
		configInput(POLLUX_DUTY_INPUT, "Pollux duty");
		configInput(CASTOR_PITCH_INPUT, "Castor pitch");
		configInput(POLLUX_PITCH_INPUT, "Pollux pitch");
		configOutput(CASTOR_MIX_OUTPUT, "Castor");
		configOutput(MIX_OUTPUT, "Mix");
		configOutput(POLLUX_MIX_OUTPUT, "Pollux");
	}


	void process(const ProcessArgs& args) override {
		float castorFreq = getPitchFreq(this->getCastorPitchCv());
		// Accumulate the phase
		castorPhase += castorFreq * args.sampleTime;
		castorSubPhase += castorFreq * args.sampleTime / 2.f;
		if (castorPhase >= 1.f) {
			castorPhase -= 2.f;
		}
		if (castorSubPhase >= 1.f) {
			castorSubPhase -= 2.f;
		}

		Signals castor = this->getSignals(castorPhase, this->getCastorDutyCycle(), castorSubPhase);
		Signals castorMix = this->getCastorMix();

		// Audio signals are typically +/-5V
		// https://vcvrack.com/manual/VoltageStandards
		float castorOut = 5.f * this->getOutput(castor, castorMix);
		outputs[CASTOR_MIX_OUTPUT].setVoltage(castorOut);

		float polluxFreq = getPitchFreq(this->getPolluxPitchCv());
		polluxPhase += polluxFreq * args.sampleTime;
		polluxSubPhase += polluxFreq * args.sampleTime / 2.f;
		if (polluxPhase >= 1.f) { 
			polluxPhase -= 2.f;
		}
		if (polluxSubPhase >= 1.f) { 
			polluxSubPhase -= 2.f;
		}

		Signals pollux = this->getSignals(polluxPhase, this->getPolluxDutyCycle(), polluxSubPhase);
		Signals polluxMix = this->getPolluxMix();
		float polluxOut = 5.f * this->getOutput(pollux, polluxMix);
		outputs[POLLUX_MIX_OUTPUT].setVoltage(polluxOut);

		outputs[MIX_OUTPUT].setVoltage(this->getMix(castorOut, polluxOut, params[CROSSFADE_PARAM].getValue()));
	}

	private:

	float getOutput(Signals wave, Signals amplitude) {
		return (wave.ramp * amplitude.ramp
			+ wave.pulse * amplitude.pulse
			+ wave.sub * amplitude.sub) / 3.f;
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

	float getLevelForOutput(ParamId parameter) {
		return rack::math::clamp(params[parameter].getValue(), 0.f, 10.f);
	}

	float ramp(float phase) { // phase \in [-1, 1)
		return -phase;
	}

	float sub(float phase) { // phase \in [-1, 1)
		return phase > 0 ? -1.f : 1.f;
	}

	// phase \in [-1, 1), duty in [0, 1)
	float pulse(float phase, float duty) {
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
		return this->getMix(CASTOR_RAMP_LEVEL_PARAM, CASTOR_PULSE_LEVEL_PARAM, CASTOR_SUB_LEVEL_PARAM);
	}

	Signals getPolluxMix() {
		return this->getMix(POLLUX_RAMP_LEVEL_PARAM, POLLUX_PULSE_LEVEL_PARAM, POLLUX_SUB_LEVEL_PARAM);
	}

	Signals getMix(ParamId ramp, ParamId pulse, ParamId sub) {
		return {
			params[ramp].getValue(),
			params[pulse].getValue(),
			params[sub].getValue(),
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

	float getSubCv(float cv) {
		return cv - 1.f;
	}

	float getPitchFreq(float cv) {
		return dsp::FREQ_C4 * std::pow(2.f, cv + 1);
	}

	float getLfoFreq(float cv) {
		// From https://vcvrack.com/manual/VoltageStandards
		// Low-frequency oscillators and clock generators should use 120 BPM
		return 2.f * std::pow(2.f, cv);
	}

	float getCastorPitchCv() {
		if (inputs[CASTOR_PITCH_INPUT].isConnected()) {
			// Return Castor pitch with a the offset from the knob.
			return inputs[CASTOR_PITCH_INPUT].getVoltage() + params[CASTOR_PITCH_PARAM].getValue();
		} else {
			// Quantize the knob output.
			// When there's no input to Castor, it has a +/- 3 Oct swing.
			float pitchCv = params[CASTOR_PITCH_PARAM].getValue() * 3.f;
			int8_t octave = static_cast<int8_t>(pitchCv);
			int8_t semitone = static_cast<int8_t>((pitchCv * 12.f) - (octave * 12));
			return static_cast<float>(octave) + static_cast<float>(semitone) / 12.f;
		}

	}

	float getPolluxPitchCv() {
		float mainPitchCv = inputs[POLLUX_PITCH_INPUT].isConnected() ?
			inputs[POLLUX_PITCH_INPUT].getVoltage() :
			this->getCastorPitchCv();

		return mainPitchCv + params[POLLUX_PITCH_PARAM].getValue();
	}
};


struct GeminiWidget : ModuleWidget {
	GeminiWidget(Gemini* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Gemini.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(16.48, 21.259)), module, Gemini::CASTOR_PITCH_PARAM));
		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(54.08, 21.259)), module, Gemini::POLLUX_PITCH_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(35.28, 42.455)), module, Gemini::LFO_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(12.331, 52.077)), module, Gemini::CASTOR_DUTY_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(58.198, 52.077)), module, Gemini::POLLUX_DUTY_PARAM));
		addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(35.28, 64.716)), module, Gemini::CROSSFADE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.854, 80.97)), module, Gemini::CASTOR_RAMP_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.536, 80.97)), module, Gemini::CASTOR_PULSE_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.447, 80.97)), module, Gemini::POLLUX_RAMP_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.536, 95.994)), module, Gemini::CASTOR_SUB_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.834, 95.994)), module, Gemini::POLLUX_SUB_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.834, 80.97)), module, Gemini::POLLUX_PULSE_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackSnapKnob>(mm2px(Vec(35.25, 88.806)), module, Gemini::BUTTON_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.854, 95.994)), module, Gemini::CASTOR_DUTY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.477, 95.994)), module, Gemini::POLLUX_DUTY_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.854, 111.12)), module, Gemini::CASTOR_PITCH_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.447, 111.12)), module, Gemini::POLLUX_PITCH_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.536, 111.12)), module, Gemini::CASTOR_MIX_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.239, 111.12)), module, Gemini::MIX_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.761, 111.12)), module, Gemini::POLLUX_MIX_OUTPUT));
	}
};


Model* modelGemini = createModel<Gemini, GeminiWidget>("Gemini");
