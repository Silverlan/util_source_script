/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SE_RESULT_HPP__
#define __SE_RESULT_HPP__

#include <cinttypes>

namespace se {
	enum class ResultCode : uint32_t {
		Ok = 0,
		Eof,
		EndOfBlock,
		Error,

		NoPhonemeData
	};
};

#endif
