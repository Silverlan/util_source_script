/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

module;

#include <sharedutils/util_markup_file.hpp>
#include <sharedutils/util.h>
#include <fsys/filesystem.h>
#include <array>

module se_script.script;

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
