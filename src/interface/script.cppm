// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

export module se_script.script;

import se_script.enums;
export import pragma.filesystem;

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
