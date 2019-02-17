/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "se_scene.hpp"
#include <sharedutils/util_markup_file.hpp>
#include <sharedutils/datastream.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util.h>
#include <fsys/filesystem.h>
#include <array>
#include <sstream>

namespace se
{
	util::MarkupFile::ResultCode read_scene(util::MarkupFile &mf,se::SceneScriptValue &root);
};

static util::MarkupFile::ResultCode read_block(util::MarkupFile &mf,se::SceneScriptValue &block,bool bRoot=false)
{
	auto token = char{};
	for(;;)
	{
		util::MarkupFile::ResultCode r {};
		if((r=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
		{
			if(bRoot == true)
				return util::MarkupFile::ResultCode::Ok;
			return util::MarkupFile::ResultCode::Error; // Missing closing bracket '}'
		}
		if(token == '}')
		{
			mf.GetDataStream()->SetOffset(mf.GetDataStream()->GetOffset() +1u);
			return util::MarkupFile::ResultCode::EndOfBlock;
		}
		auto key = std::string{};
		auto bComboBlock = false;
		if(token != '{')
		{
			if((r=mf.ReadNextString(key)) != util::MarkupFile::ResultCode::Ok)
				return (bRoot == true && r == util::MarkupFile::ResultCode::Eof) ? util::MarkupFile::ResultCode::Ok : util::MarkupFile::ResultCode::Error;
			if((r=mf.ReadNextParameterToken(token)) != util::MarkupFile::ResultCode::Ok)
				return util::MarkupFile::ResultCode::Error;
		}
		else
			bComboBlock = true;
		auto kv = std::make_shared<se::SceneScriptValue>();
		kv->identifier = key;
		block.subValues.push_back(kv);
		auto val = std::string{};
		while((r=mf.ReadNextParameterToken(token)) == util::MarkupFile::ResultCode::Ok)
		{
			if(token == '\n' || bComboBlock == true)
			{
				if(bComboBlock == false)
				{
					if((r=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
						return util::MarkupFile::ResultCode::Error;
				}
				if(token == '{')
				{
					mf.GetDataStream()->SetOffset(mf.GetDataStream()->GetOffset() +1u);
					auto r = read_block(mf,*kv);
					if(r == util::MarkupFile::ResultCode::Error)
						return util::MarkupFile::ResultCode::Error;
				}
				break;
			}
			if((r=mf.ReadNextString(val)) != util::MarkupFile::ResultCode::Ok)
				return util::MarkupFile::ResultCode::Error;
			kv->parameters.push_back(val);
		}
		if(r != util::MarkupFile::ResultCode::Ok)
			return util::MarkupFile::ResultCode::Error;
	}
	return util::MarkupFile::ResultCode::Ok;
}

util::MarkupFile::ResultCode se::read_scene(util::MarkupFile &mf,se::SceneScriptValue &block)
{
	auto r = util::MarkupFile::ResultCode{};
	while((r=read_block(mf,block,true)) == util::MarkupFile::ResultCode::EndOfBlock);
	return (r == util::MarkupFile::ResultCode::Ok || r == util::MarkupFile::ResultCode::EndOfBlock) ? util::MarkupFile::ResultCode::Ok : util::MarkupFile::ResultCode::Error;
}

util::MarkupFile::ResultCode se::read_scene(std::shared_ptr<VFilePtrInternal> &file,se::SceneScriptValue &root)
{
	auto str = file->ReadString();
	DataStream ds {};
	ds->WriteString(str);
	ds->SetOffset(0u);
	util::MarkupFile mf {ds};
	return read_scene(mf,root);
}

void se::debug_print(std::stringstream &ss,SoundPhonemeData &phonemeData)
{
	ss<<phonemeData.plainText<<":\n";
	for(auto &word : phonemeData.words)
	{
		ss<<"\t"<<word.word<<" ("<<word.tStart<<" to "<<word.tEnd<<")\n";
		for(auto &phoneme : word.phonemes)
			ss<<"\t\t"<<phoneme.phoneme<<" ("<<phoneme.tStart<<" to "<<phoneme.tEnd<<")\n";
	}
}
void se::debug_print(std::stringstream &ss,se::SceneScriptValue &val,const std::string &t)
{
	ss<<t<<"\""<<val.identifier<<"\" (";
	auto bFirst = true;
	for(auto &param : val.parameters)
	{
		if(bFirst == false)
			ss<<", ";
		ss<<"\""<<param<<"\"";
		bFirst = false;
	}
	ss<<")\n";
	for(auto &subVal : val.subValues)
		debug_print(ss,*subVal,t +"\t");
}

util::MarkupFile::ResultCode se::read_wav_phonemes(VFilePtr &wavFile,SoundPhonemeData &phonemeData)
{
	auto id = wavFile->Read<std::array<char,4>>();
	if(ustring::compare(id.data(),"RIFF",false,4u) == false)
		return util::MarkupFile::ResultCode::Error;
	wavFile->Seek(wavFile->Tell() +sizeof(uint32_t));
	id = wavFile->Read<std::array<char,4>>();
	if(ustring::compare(id.data(),"WAVE",false,4u) == false)
		return util::MarkupFile::ResultCode::Error;
	auto bListChunk = false;
	auto chunkSize = 0u;
	do
	{
		id = wavFile->Read<std::array<char,4>>();
		chunkSize = wavFile->Read<uint32_t>();
		if(wavFile->Eof())
			break;
		wavFile->Seek(wavFile->Tell() +chunkSize);
	}
	while((bListChunk = ustring::compare(id.data(),"VDAT",false,4u)) == false);
	if(bListChunk == false)
		return util::MarkupFile::ResultCode::NoPhonemeData;
	wavFile->Seek(wavFile->Tell() -chunkSize);
	auto vdat = std::string(chunkSize +1u,'\0');
	wavFile->Read(&vdat[0],chunkSize);

	DataStream ds {};
	ds->WriteString(vdat);
	ds->SetOffset(0u);

	util::MarkupFile mf {ds};
	se::SceneScriptValue sv {};
	auto r = se::read_scene(mf,sv);
	if(r != util::MarkupFile::ResultCode::Ok)
		return r;
	if(sv.subValues.empty() == true)
		return util::MarkupFile::ResultCode::NoPhonemeData;
	auto itPlainText = std::find_if(sv.subValues.begin(),sv.subValues.end(),[](const std::shared_ptr<se::SceneScriptValue> &sv) {
		return ustring::compare(sv->identifier,"PLAINTEXT",false);
	});
	if(itPlainText == sv.subValues.end() || (*itPlainText)->subValues.empty() == true)
		return util::MarkupFile::ResultCode::NoPhonemeData;

	auto &textVal = (*itPlainText)->subValues.at(0);
	phonemeData.plainText = textVal->identifier;
	for(auto &param : textVal->parameters)
		phonemeData.plainText += " " +param;

	auto itWords = std::find_if(sv.subValues.begin(),sv.subValues.end(),[](const std::shared_ptr<se::SceneScriptValue> &sv) {
		return ustring::compare(sv->identifier,"WORDS",false);
	});
	if(itWords == sv.subValues.end())
		return util::MarkupFile::ResultCode::NoPhonemeData;
	auto &wordValues = (*itWords)->subValues;
	phonemeData.words.reserve(wordValues.size());
	for(auto &wordVal : wordValues)
	{
		if(ustring::compare(wordVal->identifier,"WORD",false) == false)
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
		for(auto &phonemeValue : phonemeValues)
		{
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
	/*se::SoundPhonemeData phonemeData {};
	VFilePtr f = FileManager::OpenSystemFile("C:\\Users\\Florian\\Documents\\Projects\\weave\\x64\\Release\\sounds\\al_hazmat.wav","rb");
	if(se::read_wav_phonemes(f,phonemeData) == se::ResultCode::Ok)
	{
		std::stringstream ss {};
		se::debug_print(ss,phonemeData);
		std::cout<<ss.str()<<std::endl;
	}*/
	VFilePtr f = FileManager::OpenSystemFile("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Half-Life 2\\hl2\\scenes\\choreoexamples\\sdk_barney1.vcd","rb");

	auto str = f->ReadString();
	DataStream ds {};
	ds->WriteString(str);
	ds->SetOffset(0u);
	se::SceneScriptValue sv {};
	auto r = se::read_script(ds,sv);
	if(r == se::ResultCode::Ok)
	{
		std::stringstream ss;
		se::debug_print(ss,sv);
		std::cout<<"Data:\n"<<ss.str()<<std::endl;
	}
	for(;;);
	return 0;
}
#endif
