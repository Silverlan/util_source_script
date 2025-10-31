// SPDX-FileCopyrightText: (c) 2024 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


export module se_script.enums;

export import std.compat;

export namespace source_engine::script {
	enum class ResultCode : uint32_t {
		Ok = 0,
		Eof,
		EndOfBlock,
		Error,

		NoPhonemeData
	};
};
