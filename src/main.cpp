/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "se_script.hpp"
#include <fsys/filesystem.h>
#include <sstream>
#include <sharedutils/util_string.h>

#pragma comment(lib, "vfilesystem.lib")
#pragma comment(lib, "sharedutils.lib")
#pragma comment(lib, "mathutil.lib")

static se::ResultCode read_until(VFilePtr &f, const std::string &str, std::string &readString, bool bExclude = false)
{
	for(;;) {
		if(f->Eof())
			return se::ResultCode::Eof;
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
	return se::ResultCode::Ok;
}

static se::ResultCode read_next_token(VFilePtr &f, char &token)
{
	auto str = std::string {};
	auto r = read_until(f, ustring::WHITESPACE, str, true);
	if(r != se::ResultCode::Ok)
		return r;
	token = str.back();
	if(f->Eof())
		return se::ResultCode::Eof;
	f->Seek(f->Tell() - 1);
	return se::ResultCode::Ok;
}

static se::ResultCode read_next_string(VFilePtr &f, std::string &readStr)
{
	auto &str = readStr;
	auto r = read_until(f, ustring::WHITESPACE, str, true);
	if(r != se::ResultCode::Ok)
		return r;
	if(str.back() == '\"') {
		str.clear();
		r = read_until(f, "\"", str);
		if(r != se::ResultCode::Ok)
			return r;
		str.erase(str.end() - 1);
	}
	else {
		str.clear();
		r = read_until(f, ustring::WHITESPACE, str, true);
		if(r != se::ResultCode::Ok)
			return r;
	}
	return se::ResultCode::Ok;
}

static se::ResultCode read_block(VFilePtr &f, se::ScriptBlock &block, uint32_t blockDepth = 0u)
{
	auto token = char {};
	for(;;) {
		se::ResultCode r {};
		if((r = read_next_token(f, token)) != se::ResultCode::Ok && (r != se::ResultCode::Eof || token != '}' || blockDepth != 1u)) // If we're at eof, the last token HAS to be the } of the current block
		{
			if(blockDepth == 0u)
				return se::ResultCode::Ok;
			return se::ResultCode::Error; // Missing closing bracket '}'
		}
		if(token == '}') {
			f->Seek(f->Tell() + 1);
			return se::ResultCode::EndOfBlock;
		}
		auto key = std::string {};
		if((r = read_next_string(f, key)) != se::ResultCode::Ok)
			return se::ResultCode::Error;
		if((r = read_next_token(f, token)) != se::ResultCode::Ok)
			return se::ResultCode::Error;
		if(token == '{') {
			f->Seek(f->Tell() + 1);
			auto child = std::make_shared<se::ScriptBlock>();
			child->identifier = key;
			block.data.push_back(child);
			auto r = read_block(f, *child, blockDepth + 1u);
			if(r == se::ResultCode::Error)
				return se::ResultCode::Error;
		}
		else {
			auto val = std::string {};
			if((r = read_next_string(f, val)) != se::ResultCode::Ok)
				return se::ResultCode::Error;
			auto kv = std::make_shared<se::ScriptValue>();
			block.data.push_back(kv);
			kv->identifier = key;
			kv->value = val;
		}
	}
	return se::ResultCode::Error;
}

se::ResultCode se::read_script(VFilePtr &f, se::ScriptBlock &block)
{
	auto r = ResultCode {};
	while((r = read_block(f, block, 0u)) == ResultCode::EndOfBlock)
		;
	return (r == ResultCode::Ok || r == ResultCode::EndOfBlock) ? ResultCode::Ok : ResultCode::Error;
}

/*
static void debug_print(const se::ScriptBlock &block,std::stringstream &ss,const std::string &t="")
{
	ss<<t<<block.identifier<<"\n";
	for(auto &v : block.data)
	{
		if(v->IsBlock() == true)
		{
			auto &childBlock = static_cast<se::ScriptBlock&>(*v);
			debug_print(childBlock,ss,t +"\t");
		}
		else
		{
			auto &val = static_cast<se::ScriptValue&>(*v);
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
	auto root = std::make_shared<se::ScriptBlock>();
	auto r = se::ResultCode{};
	VFilePtr x = f;
	r = se::read_script(x,*root);
	//while((r=read_block(x,*root)) == se::ResultCode::EndOfBlock);

	std::stringstream ss {};
	debug_print(*root,ss);
	std::cout<<ss.str()<<std::endl;

	for(;;);
	return EXIT_SUCCESS;
}
*/
