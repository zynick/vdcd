//
//  utils.h
//  p44bridged
//
//  Created by Lukas Zeller on 16.04.13.
//  Copyright (c) 2013 plan44.ch. All rights reserved.
//

#ifndef __p44bridged__utils__
#define __p44bridged__utils__

#include <string>

using namespace std;

namespace p44 {

  /// printf-style format into std::string
  /// @param aFormat printf-style format string
  /// @return formatted string
  std::string string_format(const char *aFormat, ...);

  /// printf-style format appending to std::string
  /// @param aStringToAppendTo std::string to append format to
  /// @param aFormat printf-style format string
  void string_format_append(std::string &aStringToAppendTo, const char *aFormat, ...);
	
	/// always return a valid C String, if NULL is passed, an empty string is returned
	/// @param aNULLOrCStr NULL or C-String
	/// @return the input string if it is non-NULL, or an empty string
	const char *nonNullCStr(const char *aNULLOrCStr);

} // namespace p44

#endif /* defined(__p44bridged__utils__) */