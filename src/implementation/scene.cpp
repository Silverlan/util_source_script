/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sharedutils/util_markup_file.hpp>
#include <sharedutils/util.h>
#include <array>

module se_script;

namespace se_script {
	util::MarkupFile::ResultCode read_scene(util::MarkupFile &mf, SceneScriptValue &root);
};

static util::MarkupFile::ResultCode read_block(util::MarkupFile &mf, se_script::SceneScriptValue &block, bool bRoot = false)
{
	auto token = char {};
	for(;;) {
		util::MarkupFile::ResultCode r {};
		if((r = mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok) {
			if(bRoot == true)
				return util::MarkupFile::ResultCode::Ok;
			return util::MarkupFile::ResultCode::Error; // Missing closing bracket '}'
		}
		if(token == '}') {
			mf.GetDataStream()->SetOffset(mf.GetDataStream()->GetOffset() + 1u);
			return util::MarkupFile::ResultCode::EndOfBlock;
		}
		auto key = std::string {};
		auto bComboBlock = false;
		if(token != '{') {
			if((r = mf.ReadNextString(key)) != util::MarkupFile::ResultCode::Ok)
				return (bRoot == true && r == util::MarkupFile::ResultCode::Eof) ? util::MarkupFile::ResultCode::Ok : util::MarkupFile::ResultCode::Error;
			if((r = mf.ReadNextParameterToken(token)) != util::MarkupFile::ResultCode::Ok)
				return util::MarkupFile::ResultCode::Error;
		}
		else
			bComboBlock = true;
		auto kv = std::make_shared<se_script::SceneScriptValue>();
		kv->identifier = key;
		block.subValues.push_back(kv);
		auto val = std::string {};
		while((r = mf.ReadNextParameterToken(token)) == util::MarkupFile::ResultCode::Ok) {
			if(token == '\n' || bComboBlock == true) {
				if(bComboBlock == false) {
					if((r = mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
						return util::MarkupFile::ResultCode::Error;
				}
				if(token == '{') {
					mf.GetDataStream()->SetOffset(mf.GetDataStream()->GetOffset() + 1u);
					auto r = read_block(mf, *kv);
					if(r == util::MarkupFile::ResultCode::Error)
						return util::MarkupFile::ResultCode::Error;
				}
				break;
			}
			if((r = mf.ReadNextString(val)) != util::MarkupFile::ResultCode::Ok)
				return util::MarkupFile::ResultCode::Error;
			kv->parameters.push_back(val);
		}
		if(r != util::MarkupFile::ResultCode::Ok)
			return util::MarkupFile::ResultCode::Error;
	}
	return util::MarkupFile::ResultCode::Ok;
}

util::MarkupFile::ResultCode se_script::read_scene(util::MarkupFile &mf, se_script::SceneScriptValue &block)
{
	auto r = util::MarkupFile::ResultCode {};
	while((r = read_block(mf, block, true)) == util::MarkupFile::ResultCode::EndOfBlock)
		;
	return (r == util::MarkupFile::ResultCode::Ok || r == util::MarkupFile::ResultCode::EndOfBlock) ? util::MarkupFile::ResultCode::Ok : util::MarkupFile::ResultCode::Error;
}

util::MarkupFile::ResultCode se_script::read_scene(std::shared_ptr<VFilePtrInternal> &file, se_script::SceneScriptValue &root)
{
	auto str = file->ReadString();
	DataStream ds {};
	ds->WriteString(str);
	ds->SetOffset(0u);
	util::MarkupFile mf {ds};
	return read_scene(mf, root);
}

void se_script::debug_print(std::stringstream &ss, SoundPhonemeData &phonemeData)
{
	ss << phonemeData.plainText << ":\n";
	for(auto &word : phonemeData.words) {
		ss << "\t" << word.word << " (" << word.tStart << " to " << word.tEnd << ")\n";
		for(auto &phoneme : word.phonemes)
			ss << "\t\t" << phoneme.phoneme << " (" << phoneme.tStart << " to " << phoneme.tEnd << ")\n";
	}
}
void se_script::debug_print(std::stringstream &ss, se_script::SceneScriptValue &val, const std::string &t)
{
	ss << t << "\"" << val.identifier << "\" (";
	auto bFirst = true;
	for(auto &param : val.parameters) {
		if(bFirst == false)
			ss << ", ";
		ss << "\"" << param << "\"";
		bFirst = false;
	}
	ss << ")\n";
	for(auto &subVal : val.subValues)
		debug_print(ss, *subVal, t + "\t");
}

util::MarkupFile::ResultCode se_script::read_wav_phonemes(std::shared_ptr<VFilePtrInternal> &wavFile, SoundPhonemeData &phonemeData)
{
	auto id = wavFile->Read<std::array<char, 4>>();
	if(ustring::compare(id.data(), "RIFF", false, 4u) == false)
		return util::MarkupFile::ResultCode::Error;
	wavFile->Seek(wavFile->Tell() + sizeof(uint32_t));
	id = wavFile->Read<std::array<char, 4>>();
	if(ustring::compare(id.data(), "WAVE", false, 4u) == false)
		return util::MarkupFile::ResultCode::Error;
	auto bListChunk = false;
	auto chunkSize = 0u;
	do {
		id = wavFile->Read<std::array<char, 4>>();
		chunkSize = wavFile->Read<uint32_t>();
		if(wavFile->Eof())
			break;
		wavFile->Seek(wavFile->Tell() + chunkSize);
	} while((bListChunk = ustring::compare(id.data(), "VDAT", false, 4u)) == false);
	if(bListChunk == false)
		return util::MarkupFile::ResultCode::NoPhonemeData;
	wavFile->Seek(wavFile->Tell() - chunkSize);
	auto vdat = std::string(chunkSize + 1u, '\0');
	wavFile->Read(&vdat[0], chunkSize);

	DataStream ds {};
	ds->WriteString(vdat);
	ds->SetOffset(0u);

	util::MarkupFile mf {ds};
	se_script::SceneScriptValue sv {};
	auto r = se_script::read_scene(mf, sv);
	if(r != util::MarkupFile::ResultCode::Ok)
		return r;
	if(sv.subValues.empty() == true)
		return util::MarkupFile::ResultCode::NoPhonemeData;
	auto itPlainText = std::find_if(sv.subValues.begin(), sv.subValues.end(), [](const std::shared_ptr<se_script::SceneScriptValue> &sv) { return ustring::compare<std::string>(sv->identifier, "PLAINTEXT", false); });
	if(itPlainText == sv.subValues.end() || (*itPlainText)->subValues.empty() == true)
		return util::MarkupFile::ResultCode::NoPhonemeData;

	auto &textVal = (*itPlainText)->subValues.at(0);
	phonemeData.plainText = textVal->identifier;
	for(auto &param : textVal->parameters)
		phonemeData.plainText += " " + param;

	auto itWords = std::find_if(sv.subValues.begin(), sv.subValues.end(), [](const std::shared_ptr<se_script::SceneScriptValue> &sv) { return ustring::compare<std::string>(sv->identifier, "WORDS", false); });
	if(itWords == sv.subValues.end())
		return util::MarkupFile::ResultCode::NoPhonemeData;
	auto &wordValues = (*itWords)->subValues;
	phonemeData.words.reserve(wordValues.size());
	for(auto &wordVal : wordValues) {
		if(ustring::compare<std::string>(wordVal->identifier, "WORD", false) == false)
			continue;
		phonemeData.words.push_back({});
		auto &wordData = phonemeData.words.back();
		if(wordVal->parameters.size() > 0)
			wordData.word = wordVal->parameters.at(0);
		if(wordVal->parameters.size() > 1)
			wordData.tStart = util::to_float(wordVal->parameters.at(1));
		if(wordVal->parameters.size() > 2)
			wordData.tEnd = util::to_float(wordVal->parameters.at(2));

		auto &phonemeValues = wordVal->subValues;
		wordData.phonemes.reserve(phonemeValues.size());
		for(auto &phonemeValue : phonemeValues) {
			wordData.phonemes.push_back({});
			auto &phonemeData = wordData.phonemes.back();
			if(phonemeValue->parameters.size() > 0)
				phonemeData.phoneme = phonemeValue->parameters.at(0);
			if(phonemeValue->parameters.size() > 1)
				phonemeData.tStart = util::to_float(phonemeValue->parameters.at(1));
			if(phonemeValue->parameters.size() > 2)
				phonemeData.tEnd = util::to_float(phonemeValue->parameters.at(2));
		}
	}
	return util::MarkupFile::ResultCode::Ok;
}
#if 0
#include <iostream>
int main(int argc,char *argv[])
{
	/*se_script::SoundPhonemeData phonemeData {};
	VFilePtr f = FileManager::OpenSystemFile("C:\\Users\\Florian\\Documents\\Projects\\weave\\x64\\Release\\sounds\\al_hazmat.wav","rb");
	if(se_script::read_wav_phonemes(f,phonemeData) == se_script::ResultCode::Ok)
	{
		std::stringstream ss {};
		se_script::debug_print(ss,phonemeData);
		std::cout<<ss.str()<<std::endl;
	}*/
	VFilePtr f = FileManager::OpenSystemFile("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Half-Life 2\\hl2\\scenes\\choreoexamples\\sdk_barney1.vcd","rb");

	auto str = f->ReadString();
	DataStream ds {};
	ds->WriteString(str);
	ds->SetOffset(0u);
	se_script::SceneScriptValue sv {};
	auto r = se_script::read_script(ds,sv);
	if(r == se_script::ResultCode::Ok)
	{
		std::stringstream ss;
		se_script::debug_print(ss,sv);
		std::cout<<"Data:\n"<<ss.str()<<std::endl;
	}
	for(;;);
	return 0;
}
#endif

static se_script::ResultCode read_until(std::shared_ptr<VFilePtrInternal> &f, const std::string &str, std::string &readString, bool bExclude = false)
{
	for(;;) {
		if(f->Eof())
			return se_script::ResultCode::Eof;
		readString += f->ReadChar();
		auto r = str.find(readString.back());
		if((bExclude == false && r != std::string::npos) || (bExclude == true && r == std::string::npos)) {
			if(readString.back() != '/')
				break;
			auto peek = f->ReadChar();
			if(peek == '/') {
				readString.erase(readString.end() - 1);
				f->ReadLine();
			}
			else {
				f->Seek(f->Tell() - 1);
				break;
			}
		}
	}
	return se_script::ResultCode::Ok;
}

static se_script::ResultCode read_next_token(std::shared_ptr<VFilePtrInternal> &f, char &token)
{
	auto str = std::string {};
	auto r = read_until(f, ustring::WHITESPACE, str, true);
	if(r != se_script::ResultCode::Ok)
		return r;
	token = str.back();
	if(f->Eof())
		return se_script::ResultCode::Eof;
	f->Seek(f->Tell() - 1);
	return se_script::ResultCode::Ok;
}

static se_script::ResultCode read_next_string(std::shared_ptr<VFilePtrInternal> &f, std::string &readStr)
{
	auto &str = readStr;
	auto r = read_until(f, ustring::WHITESPACE, str, true);
	if(r != se_script::ResultCode::Ok)
		return r;
	if(str.back() == '\"') {
		str.clear();
		r = read_until(f, "\"", str);
		if(r != se_script::ResultCode::Ok)
			return r;
		str.erase(str.end() - 1);
	}
	else {
		str.clear();
		r = read_until(f, ustring::WHITESPACE, str, true);
		if(r != se_script::ResultCode::Ok)
			return r;
	}
	return se_script::ResultCode::Ok;
}

static se_script::ResultCode read_block(std::shared_ptr<VFilePtrInternal> &f, se_script::ScriptBlock &block, uint32_t blockDepth = 0u)
{
	auto token = char {};
	for(;;) {
		se_script::ResultCode r {};
		if((r = read_next_token(f, token)) != se_script::ResultCode::Ok && (r != se_script::ResultCode::Eof || token != '}' || blockDepth != 1u)) // If we're at eof, the last token HAS to be the } of the current block
		{
			if(blockDepth == 0u)
				return se_script::ResultCode::Ok;
			return se_script::ResultCode::Error; // Missing closing bracket '}'
		}
		if(token == '}') {
			f->Seek(f->Tell() + 1);
			return se_script::ResultCode::EndOfBlock;
		}
		auto key = std::string {};
		if((r = read_next_string(f, key)) != se_script::ResultCode::Ok)
			return se_script::ResultCode::Error;
		if((r = read_next_token(f, token)) != se_script::ResultCode::Ok)
			return se_script::ResultCode::Error;
		if(token == '{') {
			f->Seek(f->Tell() + 1);
			auto child = std::make_shared<se_script::ScriptBlock>();
			child->identifier = key;
			block.data.push_back(child);
			auto r = read_block(f, *child, blockDepth + 1u);
			if(r == se_script::ResultCode::Error)
				return se_script::ResultCode::Error;
		}
		else {
			auto val = std::string {};
			if((r = read_next_string(f, val)) != se_script::ResultCode::Ok)
				return se_script::ResultCode::Error;
			auto kv = std::make_shared<se_script::ScriptValue>();
			block.data.push_back(kv);
			kv->identifier = key;
			kv->value = val;
		}
	}
	return se_script::ResultCode::Error;
}

se_script::ResultCode se_script::read_script(std::shared_ptr<VFilePtrInternal> &f, se_script::ScriptBlock &block)
{
	auto r = ResultCode {};
	while((r = read_block(f, block, 0u)) == ResultCode::EndOfBlock)
		;
	return (r == ResultCode::Ok || r == ResultCode::EndOfBlock) ? ResultCode::Ok : ResultCode::Error;
}

/*
static void debug_print(const se_script::ScriptBlock &block,std::stringstream &ss,const std::string &t="")
{
	ss<<t<<block.identifier<<"\n";
	for(auto &v : block.data)
	{
		if(v->IsBlock() == true)
		{
			auto &childBlock = static_cast<se_script::ScriptBlock&>(*v);
			debug_print(childBlock,ss,t +"\t");
		}
		else
		{
			auto &val = static_cast<se_script::ScriptValue&>(*v);
			ss<<t<<"\t"<<val.identifier<<" = "<<val.value<<"\n";
		}
	}
}

#include <iostream>
int main(int argc,char *argv[])
{
	auto f = FileManager::OpenSystemFile("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Source\\hl2\\scripts\\game_sounds_vehicles.txt","r");
	if(f == nullptr)
		return EXIT_SUCCESS;
	auto root = std::make_shared<se_script::ScriptBlock>();
	auto r = se_script::ResultCode{};
	VFilePtr x = f;
	r = se_script::read_script(x,*root);
	//while((r=read_block(x,*root)) == se_script::ResultCode::EndOfBlock);

	std::stringstream ss {};
	debug_print(*root,ss);
	std::cout<<ss.str()<<std::endl;

	for(;;);
	return EXIT_SUCCESS;
}
*/
