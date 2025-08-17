#include "plugin.hpp"


struct Gemini : Module {
	enum ParamId {
		CASTOR_PITCH_PARAM_PARAM,
		POLLUX_PITCH_PARAM_PARAM,
		LFO_PARAM,
		CASTOR_DUTY_PARAM_PARAM,
		POLLUX_DUTY_PARAM_PARAM,
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
		CASTOR_DUTY_IN_INPUT,
		POLLUX_DUTY_IN_INPUT,
		CASTOR_PITCH_IN_INPUT,
		POLLUX_PITCH_IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		CASTOR_MIX_OUT_OUTPUT,
		MIX_OUT_OUTPUT,
		POLLUX_MIX_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	Gemini() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CASTOR_PITCH_PARAM_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POLLUX_PITCH_PARAM_PARAM, 0.f, 1.f, 0.f, "");
		configParam(LFO_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CASTOR_DUTY_PARAM_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POLLUX_DUTY_PARAM_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CROSSFADE_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CASTOR_RAMP_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CASTOR_PULSE_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POLLUX_PULSE_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(BUTTON_PARAM, 0.f, 1.f, 0.f, "");
		configParam(CASTOR_SUB_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POLLUX_SUB_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configParam(POLLUX_RAMP_LEVEL_PARAM, 0.f, 1.f, 0.f, "");
		configInput(CASTOR_DUTY_IN_INPUT, "");
		configInput(POLLUX_DUTY_IN_INPUT, "");
		configInput(CASTOR_PITCH_IN_INPUT, "");
		configInput(POLLUX_PITCH_IN_INPUT, "");
		configOutput(CASTOR_MIX_OUT_OUTPUT, "");
		configOutput(MIX_OUT_OUTPUT, "");
		configOutput(POLLUX_MIX_OUT_OUTPUT, "");
	}

	void process(const ProcessArgs& args) override {
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.48, 21.259)), module, Gemini::CASTOR_PITCH_PARAM_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(54.08, 21.259)), module, Gemini::POLLUX_PITCH_PARAM_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.28, 42.455)), module, Gemini::LFO_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(12.331, 52.077)), module, Gemini::CASTOR_DUTY_PARAM_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(58.198, 52.077)), module, Gemini::POLLUX_DUTY_PARAM_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.28, 64.716)), module, Gemini::CROSSFADE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.854, 80.97)), module, Gemini::CASTOR_RAMP_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.536, 80.97)), module, Gemini::CASTOR_PULSE_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(62.447, 80.97)), module, Gemini::POLLUX_PULSE_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(35.25, 88.806)), module, Gemini::BUTTON_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(21.536, 95.994)), module, Gemini::CASTOR_SUB_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(48.834, 95.994)), module, Gemini::POLLUX_SUB_LEVEL_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(1288.93, 649.904)), module, Gemini::POLLUX_RAMP_LEVEL_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.854, 95.994)), module, Gemini::CASTOR_DUTY_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.477, 95.994)), module, Gemini::POLLUX_DUTY_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.854, 111.12)), module, Gemini::CASTOR_PITCH_IN_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(62.447, 111.12)), module, Gemini::POLLUX_PITCH_IN_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.536, 111.12)), module, Gemini::CASTOR_MIX_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(35.239, 111.12)), module, Gemini::MIX_OUT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.761, 111.12)), module, Gemini::POLLUX_MIX_OUT_OUTPUT));
	}
};


Model* modelGemini = createModel<Gemini, GeminiWidget>("Gemini");