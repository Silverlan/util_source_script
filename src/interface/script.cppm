/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

module;

#include <string>
#include <vector>
#include <memory>
#include <fsys/filesystem.h>

export module se_script.script;

import se_script.enums;

export namespace source_engine::script {
	struct ScriptBase {
		std::string identifier {};
		virtual bool IsBlock() const = 0;
	};

	struct ScriptValue : public ScriptBase {
		std::string value {};
		virtual bool IsBlock() const override { return false; }
	};

	struct ScriptBlock : public ScriptBase {
		std::vector<std::shared_ptr<ScriptBase>> data {};
		virtual bool IsBlock() const override { return true; }
	};

	ResultCode read_script(std::shared_ptr<VFilePtrInternal> &f, ScriptBlock &root);
};
