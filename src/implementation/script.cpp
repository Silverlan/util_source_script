// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module se_script.script;

static source_engine::script::ResultCode read_until(std::shared_ptr<VFilePtrInternal> &f, const std::string &str, std::string &readString, bool bExclude = false)
{
	for(;;) {
		if(f->Eof())
			return source_engine::script::ResultCode::Eof;
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
	return source_engine::script::ResultCode::Ok;
}

static source_engine::script::ResultCode read_next_token(std::shared_ptr<VFilePtrInternal> &f, char &token)
{
	auto str = std::string {};
	auto r = read_until(f, ustring::WHITESPACE, str, true);
	if(r != source_engine::script::ResultCode::Ok)
		return r;
	token = str.back();
	if(f->Eof())
		return source_engine::script::ResultCode::Eof;
	f->Seek(f->Tell() - 1);
	return source_engine::script::ResultCode::Ok;
}

static source_engine::script::ResultCode read_next_string(std::shared_ptr<VFilePtrInternal> &f, std::string &readStr)
{
	auto &str = readStr;
	auto r = read_until(f, ustring::WHITESPACE, str, true);
	if(r != source_engine::script::ResultCode::Ok)
		return r;
	if(str.back() == '\"') {
		str.clear();
		r = read_until(f, "\"", str);
		if(r != source_engine::script::ResultCode::Ok)
			return r;
		str.erase(str.end() - 1);
	}
	else {
		str.clear();
		r = read_until(f, ustring::WHITESPACE, str, true);
		if(r != source_engine::script::ResultCode::Ok)
			return r;
	}
	return source_engine::script::ResultCode::Ok;
}

static source_engine::script::ResultCode read_block(std::shared_ptr<VFilePtrInternal> &f, source_engine::script::ScriptBlock &block, uint32_t blockDepth = 0u)
{
	auto token = char {};
	for(;;) {
		source_engine::script::ResultCode r {};
		if((r = read_next_token(f, token)) != source_engine::script::ResultCode::Ok && (r != source_engine::script::ResultCode::Eof || token != '}' || blockDepth != 1u)) // If we're at eof, the last token HAS to be the } of the current block
		{
			if(blockDepth == 0u)
				return source_engine::script::ResultCode::Ok;
			return source_engine::script::ResultCode::Error; // Missing closing bracket '}'
		}
		if(token == '}') {
			f->Seek(f->Tell() + 1);
			return source_engine::script::ResultCode::EndOfBlock;
		}
		auto key = std::string {};
		if((r = read_next_string(f, key)) != source_engine::script::ResultCode::Ok)
			return source_engine::script::ResultCode::Error;
		if((r = read_next_token(f, token)) != source_engine::script::ResultCode::Ok)
			return source_engine::script::ResultCode::Error;
		if(token == '{') {
			f->Seek(f->Tell() + 1);
			auto child = std::make_shared<source_engine::script::ScriptBlock>();
			child->identifier = key;
			block.data.push_back(child);
			auto r = read_block(f, *child, blockDepth + 1u);
			if(r == source_engine::script::ResultCode::Error)
				return source_engine::script::ResultCode::Error;
		}
		else {
			auto val = std::string {};
			if((r = read_next_string(f, val)) != source_engine::script::ResultCode::Ok)
				return source_engine::script::ResultCode::Error;
			auto kv = std::make_shared<source_engine::script::ScriptValue>();
			block.data.push_back(kv);
			kv->identifier = key;
			kv->value = val;
		}
	}
	return source_engine::script::ResultCode::Error;
}

source_engine::script::ResultCode source_engine::script::read_script(std::shared_ptr<VFilePtrInternal> &f, source_engine::script::ScriptBlock &block)
{
	auto r = ResultCode {};
	while((r = read_block(f, block, 0u)) == ResultCode::EndOfBlock)
		;
	return (r == ResultCode::Ok || r == ResultCode::EndOfBlock) ? ResultCode::Ok : ResultCode::Error;
}

/*
static void debug_print(const source_engine::script::ScriptBlock &block,std::stringstream &ss,const std::string &t="")
{
	ss<<t<<block.identifier<<"\n";
	for(auto &v : block.data)
	{
		if(v->IsBlock() == true)
		{
			auto &childBlock = static_cast<source_engine::script::ScriptBlock&>(*v);
			debug_print(childBlock,ss,t +"\t");
		}
		else
		{
			auto &val = static_cast<source_engine::script::ScriptValue&>(*v);
			ss<<t<<"\t"<<val.identifier<<" = "<<val.value<<"\n";
		}
	}
}

int main(int argc,char *argv[])
{
	auto f = FileManager::OpenSystemFile("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Counter-Strike Source\\hl2\\scripts\\game_sounds_vehicles.txt","r");
	if(f == nullptr)
		return EXIT_SUCCESS;
	auto root = std::make_shared<source_engine::script::ScriptBlock>();
	auto r = source_engine::script::ResultCode{};
	VFilePtr x = f;
	r = source_engine::script::read_script(x,*root);
	//while((r=read_block(x,*root)) == source_engine::script::ResultCode::EndOfBlock);

	std::stringstream ss {};
	debug_print(*root,ss);
	std::cout<<ss.str()<<std::endl;

	for(;;);
	return EXIT_SUCCESS;
}
*/
