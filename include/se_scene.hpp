/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SE_SCENE_HPP__
#define __SE_SCENE_HPP__

#include <string>
#include <vector>
#include <memory>
#include <sharedutils/util_markup_file.hpp>

class VFilePtrInternal;
namespace se
{
	struct SceneScriptValue
	{
		std::string identifier = {};
		std::vector<std::string> parameters = {};
		std::vector<std::shared_ptr<SceneScriptValue>> subValues = {};
	};

	struct SoundPhonemeData
	{
		struct PhonemeData
		{
			PhonemeData()=default;
			PhonemeData(const std::string &ph,float start,float end)
				: phoneme(ph),tStart(start),tEnd(end)
			{}
			std::string phoneme = {};
			float tStart = 0.f;
			float tEnd = 0.f;
		};
		struct WordData
		{
			WordData()=default;
			WordData(const std::string &w,float start,float end)
				: word(w),tStart(start),tEnd(end)
			{}
			std::string word = {};
			float tStart = 0.f;
			float tEnd = 0.f;
			std::vector<PhonemeData> phonemes = {};
		};
		std::string plainText = {};
		std::vector<WordData> words = {};
	};

	util::MarkupFile::ResultCode read_wav_phonemes(std::shared_ptr<VFilePtrInternal> &wavFile,SoundPhonemeData &phonemeData);
	util::MarkupFile::ResultCode read_scene(std::shared_ptr<VFilePtrInternal> &file,se::SceneScriptValue &root);

	void debug_print(std::stringstream &ss,SoundPhonemeData &phonemeData);
	void debug_print(std::stringstream &ss,se::SceneScriptValue &val,const std::string &t="");
};

#endif
