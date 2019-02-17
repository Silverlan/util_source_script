/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SE_SCRIPT_HPP__
#define __SE_SCRIPT_HPP__

#include <string>
#include <vector>
#include <memory>
#include "se_result.hpp"

class VFilePtrInternal;
namespace se
{
	struct ScriptBase
	{
		std::string identifier {};
		virtual bool IsBlock() const=0;
	};

	struct ScriptValue
		: public ScriptBase
	{
		std::string value {};
		virtual bool IsBlock() const override {return false;}
	};

	struct ScriptBlock
		: public ScriptBase
	{
		std::vector<std::shared_ptr<ScriptBase>> data {};
		virtual bool IsBlock() const override {return true;}
	};

	ResultCode read_script(std::shared_ptr<VFilePtrInternal> &f,se::ScriptBlock &root);
};

#endif
