//! @file String.hpp

#pragma once

#undef min
#undef max

#ifdef PLATFORM_LINUX
#include <string.h>
//Utter bodge to make things work because of Windows.
inline char* strncpy_s(char *strDest, Uint32 numberOfElements, const char *strSource, Uint32 count)
{
	return strncpy(strDest, strSource, count);
}
#endif

//! Performs data and methods to operate a string buffer
class String {
public:
	String() = default;
	String(const String& src) {
		assign(src.get());
	}
	String(const char* src) {
		assign(src);
	}
	virtual ~String() {
		if (str) {
			free(str);
			str = nullptr;
			size = 0;
		}
	}

	//! represents an invalid position in the string
	static const Uint32 npos = UINT32_MAX;

	const char*				get() const { return str ? str : ""; }
	const Uint32			getSize() const { return size; }

	//! allocs / reallocs the string (erases data!)
	//! @param newSize the size (in chars) of the string to alloc
	//! @return the alloc'd string
	virtual const char* alloc(const Uint32 newSize) {
		size = newSize;
		if (str) {
			char* result = (char*)realloc(str, size * sizeof(char));
			assert(result);
			str = result;
		} else {
			str = (char*)malloc(size * sizeof(char));
			assert(str);
		}
		str[0] = '\0';
		str[size - 1] = '\0';
		return str;
	}

	//! determine if the string is empty
	//! @return true if empty, otherwise false
	bool empty() const {
		if (str) {
			return str[0] == '\0';
		} else {
			return true;
		}
	}

	//! get the length of the string
	//! @return the number of chars in the string
	Uint32 length() const {
		if (str == nullptr || size == 0) {
			return 0;
		}
		Uint32 c = 0;
		for (; c < size && str[c] != '\0'; ++c);
		return c;
	}

	//! copies src into string. string will always be null-terminated
	//! @param src the string to copy into our string
	const char* assign(const char* src) {
		if (src == nullptr) {
			if (str) {
				str[0] = '\0';
				return str;
			} else {
				return "";
			}
		} else {
			if (str == src || *this == src) {
				return str;
			}
			Uint32 srcLen = static_cast<Uint32>(strlen(src)) + 1;
			if (srcLen > size || str == nullptr) {
				alloc(srcLen);
			}
			assert(str);
			strncpy_s(str, size, src, size - 1);
			str[size - 1] = '\0';
			return str;
		}
	}

	//! copies formatted src string into our string. string will always be null-terminated
	//! @param src the formatted string to copy
	//! @return the formatted string
	const char* format(const char* src, ...) {
		if (str == nullptr || size == 0) {
			return "";
		}
		if (src == nullptr) {
			str[0] = '\0';
			return str;
		}
		va_list argptr;
		va_start(argptr, src);
		vsnprintf(str, size, src, argptr);
		va_end(argptr);
		str[size - 1] = '\0';

		return str;
	}

	//! append formatted string into our string. string will always be null-terminated
	//! @param src the formatted string to append
	//! @return this string
	const char* append(const char* src) {
		if (str == nullptr) {
			return "";
		}
		if (src == nullptr) {
			return str;
		}
		Uint32 len = length();
		if (len >= size - 1) {
			return str;
		}
		Uint32 last = std::min(len + (Uint32)strlen(src), size - 1);

		strncpy_s((char *)(str + len), size - len, src, last - len);
		str[last] = '\0';
		return str;
	}

	//! append formatted string into our string. string will always be null-terminated
	//! @param src the formatted string to append
	//! @return this string
	const char* appendf(const char* src, ...) {
		if (str == nullptr) {
			return "";
		}
		if (src == nullptr) {
			return str;
		}
		Uint32 len = length();
		if (len >= size - 1) {
			return str;
		}

		char* buf = (char*)malloc((size - len) * sizeof(char));
		assert(buf);
		buf[0] = '\0';
		buf[size - len - 1] = '\0';
		va_list argptr;
		va_start(argptr, src);
		vsnprintf(buf, size - len, src, argptr);
		va_end(argptr);
		buf[size - len - 1] = '\0';

		strncpy_s((char *)(str + len), size - len, buf, size - len - 1);
		str[size - 1] = '\0';
		free(buf);

		return str;
	}

	//! build a new string using part of this string
	//! @param start the index to start at
	//! @param end the index to end at
	//! @return the resulting substr
	String substr(const Uint32 start, Uint32 end = 0) const {
		if (end == 0) {
			end = size;
		}
		if (str == nullptr) {
			return String("");
		}

		end = std::min(end, length());

		if (start >= end) {
			return String("");
		}

		Uint32 c, i;
		String result;
		result.alloc(end - start + 1);
		for (i = 0, c = start; c < end; ++c, ++i) {
			result[i] = str[c];
		}
		result[end - start] = '\0';
		return result;
	}

	//! locates the first instance of the key string in our string and returns its location
	//! @param key the string to locate
	//! @param offset the index to start looking in our string
	//! @return the index of the key if it was found, or npos if it could not be found
	Uint32 find(const char* key, Uint32 offset = 0) const {
		if (key == nullptr || str == nullptr) {
			return npos;
		}
		Uint32 ourLen = length();
		Uint32 itsLen = static_cast<Uint32>(strlen(key));
		for (Uint32 c = offset; c < ourLen; ++c) {
			Uint32 i, j;
			for (j = 0, i = c; i < ourLen && j < itsLen; ++i, ++j) {
				if (str[i] != key[j]) {
					break;
				}
			}
			if (j == itsLen) {
				return c;
			}
		}
		return npos;
	}

	//! locates the first instance of the key char in our string and returns its location
	//! @param key the char to locate
	//! @param offset the index to start looking in our string
	//! @return the index of the key if it was found, or npos if it could not be found
	Uint32 find(const char key, Uint32 offset = 0) const {
		if (str == nullptr) {
			return npos;
		}
		Uint32 ourLen = length();
		for (Uint32 c = offset; c < ourLen; ++c) {
			if (str[c] == key) {
				return c;
			}
		}
		return npos;
	}

	//! converts the string to an integer number
	//! @return the int value
	int toInt() const {
		int result = 0;
		if (str != nullptr) {
			result = strtol(str, nullptr, 10);
		}
		return result;
	}

	//! converts the string to a floating point number
	//! @return the float value
	float toFloat() const {
		float result = 0.f;
		if (str != nullptr) {
			result = strtof(str, nullptr);
		}
		return result;
	}

	//! hash the string
	//! @return a number representation of the string
	unsigned long hash() const {
		if (str == nullptr) {
			return 0;
		}
		unsigned long value = 5381;
		int c;
		const char* data = str;
		while ((c = *data++) != 0) {
			value = ((value << 5) + value) + c; //! hash * 33 + c
		}
		return value;
	}

	//! access char data
	//! @return the char at the given index
	char& operator[](const Uint32 index) {
		assert(index >= 0 && index < size);
		return str[index];
	}
	const char& operator[](const Uint32 index) const {
		assert(index >= 0 && index < size);
		return str[index];
	}

	//! compare strings
	bool operator<(const char* src) const {
		if (str && src) {
			return strcmp(str, src) < 0;
		} else if (!str && !src) {
			return true;
		} else {
			return false;
		}
	}
	bool operator<=(const char* src) const {
		if (str && src) {
			return strcmp(str, src) <= 0;
		} else if (!str && !src) {
			return true;
		} else {
			return false;
		}
	}
	bool operator==(const char* src) const {
		if (str && src) {
			return strcmp(str, src) == 0;
		} else if (!str && !src) {
			return true;
		} else {
			return false;
		}
	}
	bool operator>=(const char* src) const {
		if (str && src) {
			return strcmp(str, src) >= 0;
		} else if (!str && !src) {
			return false;
		} else {
			return true;
		}
	}
	bool operator>(const char* src) const {
		if (str && src) {
			return strcmp(str, src) > 0;
		} else if (!str && !src) {
			return false;
		} else {
			return true;
		}
	}
	bool operator!=(const char* src) const {
		if (str && src) {
			return strcmp(str, src) != 0;
		} else if (!str && !src) {
			return false;
		} else {
			return true;
		}
	}

	//! assign operator
	String& operator=(const char* src) {
		assign(src);
		return *this;
	}
	String& operator=(const String& src) {
		assign(src.get());
		return *this;
	}

	//! move operator
	String& operator=(String&& src) {
		swap(std::move(src));
		return *this;
	}

	//! conversion to const char*
	operator const char*() const {
		return get();
	}

	//! swap contents of this string with those of another
	//! @param src other string
	virtual void swap(String&& src) {
		auto tstr = str;
		auto tsize = size;
		str = src.str;
		size = src.size;
		src.str = tstr;
		src.size = tsize;
	}

protected:
	char* str = nullptr;
	Uint32 size = 0;
};

//! Like a normal string, but is allocated on the stack instead of the heap.
template<Uint32 defaultSize>
class StringBuf : public String {
public:
	StringBuf() {
		size = defaultSize;
		str = defaultStr;
	}
	StringBuf(const StringBuf& src) {
		assign(src.get());
	}
	StringBuf(const String& src) {
		assign(src.get());
	}
	StringBuf(String&&) = delete;
	StringBuf(StringBuf&&) = delete;
	StringBuf(const char* src) {
		size = defaultSize;
		str = defaultStr;
		strncpy_s(str, size, src, size - 1);
		str[size - 1] = '\0';
	}
	StringBuf(const char* src, int numArgs, ...) {
		size = defaultSize;
		str = defaultStr;

		if (numArgs == 0) {
			strncpy_s(str, size, src, size - 1);
			str[size - 1] = '\0';
		} else {
			str[0] = '\0';
			str[size - 1] = '\0';

			va_list argptr;
			va_start(argptr, numArgs);
			vsnprintf(str, size, src, argptr);
			va_end(argptr);

			str[size - 1] = '\0';
		}
	}
	virtual ~StringBuf() {
		if (str == defaultStr) {
			str = nullptr;
		}
	}

	StringBuf& operator=(const char* src) {
		assign(src);
		return *this;
	}
	StringBuf& operator=(const String& src) {
		assign(src.get());
		return *this;
	}
	StringBuf& operator=(const StringBuf& src) {
		assign(src.get());
		return *this;
	}

	StringBuf& operator=(StringBuf&&) = delete;
	StringBuf& operator=(String&&) = delete;

	//! allocs / reallocs the string (erases data!)
	//! @param newSize the size (in chars) of the string to alloc
	//! @return the alloc'd string
	virtual const char* alloc(const Uint32 newSize) override {
		size = newSize;
		if (str == defaultStr) {
			str = nullptr;
		}
		if (str) {
			char* result = (char*)realloc(str, size * sizeof(char));
			assert(result);
			str = result;
		} else {
			str = (char*)malloc(size * sizeof(char));
			assert(str);
		}
		str[0] = '\0';
		str[size - 1] = '\0';
		return str;
	}

	virtual void swap(String&& src) override {
		assert(0 && "StringBuf::swap() is not supported");
	}

	static Uint32 getDefaultSize() { return defaultSize; }

protected:
	char defaultStr[defaultSize] = "\0";
};