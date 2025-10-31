// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


export module se_script.scene;

export import pragma.filesystem;

export namespace source_engine::script {
	struct SceneScriptValue {
		std::string identifier = {};
		std::vector<std::string> parameters = {};
		std::vector<std::shared_ptr<SceneScriptValue>> subValues = {};
	};

	struct SoundPhonemeData {
		struct PhonemeData {
			PhonemeData() = default;
			PhonemeData(const std::string &ph, float start, float end) : phoneme(ph), tStart(start), tEnd(end) {}
			std::string phoneme = {};
			float tStart = 0.f;
			float tEnd = 0.f;
		};
		struct WordData {
			WordData() = default;
			WordData(const std::string &w, float start, float end) : word(w), tStart(start), tEnd(end) {}
			std::string word = {};
			float tStart = 0.f;
			float tEnd = 0.f;
			std::vector<PhonemeData> phonemes = {};
		};
		std::string plainText = {};
		std::vector<WordData> words = {};
	};

	util::MarkupFile::ResultCode read_wav_phonemes(std::shared_ptr<VFilePtrInternal> &wavFile, SoundPhonemeData &phonemeData);
	util::MarkupFile::ResultCode read_scene(std::shared_ptr<VFilePtrInternal> &file, SceneScriptValue &root);

	void debug_print(std::stringstream &ss, SoundPhonemeData &phonemeData);
	void debug_print(std::stringstream &ss, SceneScriptValue &val, const std::string &t = "");
};
