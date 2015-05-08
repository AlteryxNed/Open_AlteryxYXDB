#pragma once
#define _SCL_SECURE_NO_DEPRECATE
#if !defined(UNICODE) 
#error UNICODE must be defined
#endif
#include <assert.h>
#include <string>
#define SRCLIB_REPLACEMENT

#define	sizeofArray(a)		(sizeof(a) / sizeof(a[0]))

namespace SRC
{
	inline const size_t & src_min_size_t(const size_t &a, const size_t &b)
	{
		return (b<a) ? b : a;
	} // src_min


	inline double ConvertToDouble(const char *p)
	{
		return atof(p);
	}
	inline double ConvertToDouble(const wchar_t *p)
	{
		return _wtof(p);
	}
	// these variants return the # of characters used
	inline unsigned ConvertToDouble(const char *p, double &d, bool &)
	{
		char *pEnd;
		d = strtod(p, &pEnd);
		return unsigned(pEnd-p);
	}
	inline unsigned ConvertToDouble(const wchar_t *p, double &d, bool &)
	{
		wchar_t *pEnd;
		d = wcstod(p, &pEnd);
		return unsigned(pEnd-p);
	}
	template< typename CharType >
	inline unsigned ConvertToDouble( const CharType * const data, double& outVal )
	{
		bool bOverflow;
		return ConvertToDouble(data, outVal, bOverflow);
	}

	inline int ConvertToInt(const char *p)
	{
		return atoi(p);
	}
	inline int ConvertToInt(const wchar_t *p)
	{
		return _wtoi(p);
	}
	inline __int64 ConvertToInt64(const char *p)
	{
		return _atoi64(p);
	}
	inline __int64 ConvertToInt64(const wchar_t *p)
	{
		return _wtoi64(p);
	}

	template<class TChar, class T_char_traits = std::char_traits<TChar> >
	class Tstr 
		: public std::basic_string<TChar, T_char_traits>
	{
		static inline void Assign(char *pBuffer, int n)
		{
			_itoa(n, pBuffer, 10);
		}
		static inline void Assign(wchar_t *pBuffer, int n)
		{
			_itow(n, pBuffer, 10);
		}
		static inline void Assign(char *pBuffer, __int64 n)
		{
			_i64toa(n, pBuffer, 10);
		}
		static inline void Assign(wchar_t *pBuffer, __int64 n)
		{
			_i64tow(n, pBuffer, 10);
		}
		static inline void Assign(char *pBuffer, unsigned nBufferSize, double d, int iDecimals)
		{
			_snprintf(pBuffer, nBufferSize, "%.*f", iDecimals, d);
		}
		static inline void Assign(wchar_t *pBuffer, unsigned nBufferSize, double d, int iDecimals)
		{
			_snwprintf(pBuffer, nBufferSize, L"%.*f", iDecimals, d);
		}
	public:
		inline Tstr()
		{}
		inline Tstr(const TChar * p)
			: std::basic_string<TChar, T_char_traits>(p)
		{
		}

		Tstr(const TChar* p, int len)
			: std::basic_string<TChar, T_char_traits>(p,len)
		{ }

		inline unsigned Length() const { return unsigned(std::basic_string<TChar, T_char_traits>::length()); }

		inline bool IsEmpty() const { return std::basic_string<TChar, T_char_traits>::empty(); }

		Tstr& Assign(const TChar* p, int len)
		{
			std::basic_string<TChar, T_char_traits>::assign(p,size_t(len));
			return *this;
		}

		inline TChar * Lock(int nLen = -1)
		{
			if (nLen<0)
				nLen = unsigned(Length());
			Tstr<TChar, T_char_traits> dest;
			dest.resize(nLen);
			memcpy(const_cast<TChar *>(dest.c_str()), std::basic_string<TChar, T_char_traits>::c_str(), std::min(unsigned(nLen), Length()+1)*sizeof(TChar));
			*this = dest;
			return const_cast<TChar *>(std::basic_string<TChar, T_char_traits>::c_str());
		}
		inline void Unlock()
		{
			const TChar *pBegin = std::basic_string<TChar, T_char_traits>::c_str();
			const TChar *pEnd = pBegin;
			while(*pEnd)
				pEnd++;

			resize(pEnd-pBegin);
		}
		inline void Unlock(unsigned nLen)
		{
			std::basic_string<TChar, T_char_traits>::resize(nLen);
		}
		inline operator const TChar *() const
		{
			return std::basic_string<TChar, T_char_traits>::c_str();
		}
		inline Tstr<TChar, T_char_traits> ReplaceString(const Tstr<TChar, T_char_traits> &target, const Tstr<TChar, T_char_traits> &replacement) const
		{
			Tstr<TChar, T_char_traits> dest = *this;
			typename std::basic_string<TChar, T_char_traits>::size_type pos = 0;
			while ((pos=dest.find(target, pos))!=std::basic_string<TChar, T_char_traits>::npos)
			{
				dest.replace(pos, target.length(), replacement);
				pos += target.length();
			}

			return dest;
		}
		inline Tstr<TChar, T_char_traits> & Assign(int n)
		{
			TChar buffer[256];
			Assign(buffer, n);
			*this = buffer;
			return *this;
		}
		inline Tstr<TChar, T_char_traits> & Assign(__int64 n)
		{
			TChar buffer[256];
			Assign(buffer, n);
			*this = buffer;
			return *this;
		}
		Tstr<TChar> & Assign(const double d)
		{
			TChar buffer[_CVTBUFSIZE];
			Assign(buffer, _CVTBUFSIZE, d, 6);

			TChar * pEnd = buffer;
			while (*pEnd)
				pEnd++;
			--pEnd;

			while (*pEnd=='0')
			{
				*pEnd = 0;
				--pEnd;
			}
			if (*pEnd=='.')
			{
				*pEnd = 0;
				--pEnd;
			}

			*this = buffer;
			return *this;
		}
		inline Tstr<TChar, T_char_traits> & Assign(double d, int iDecimals)
		{
			TChar buffer[_CVTBUFSIZE];
			Assign(buffer, _CVTBUFSIZE, d, iDecimals);
			*this = buffer;
			return *this;
		}
		inline double ConvertToDouble() const
		{
			return ::SRC::ConvertToDouble(std::basic_string<TChar, T_char_traits>::c_str());
		}

		inline int ConvertToInt() const
		{
			return ::SRC::ConvertToInt(std::basic_string<TChar, T_char_traits>::c_str());
		}
		inline __int64 ConvertToInt64() const
		{
			return ::SRC::ConvertToInt64(std::basic_string<TChar, T_char_traits>::c_str());
		}

		inline void Truncate(unsigned n)
		{
			if (n<Length())
				std::basic_string<TChar, T_char_traits>::resize(n);
		}
		inline void Append(const TChar *p, size_t nLen)
		{
			append(p, nLen);
		}
		inline void Append(const TChar *p)
		{
			const TChar *pEnd = p;
			while(*pEnd)
				pEnd++;

			append(p, pEnd-p);
		}

		inline Tstr<TChar, T_char_traits> & TrimLeft()
		{
			while (!IsEmpty() && isspace(*(std::basic_string<TChar, T_char_traits>::end() - 1)))
				std::basic_string<TChar, T_char_traits>::resize(std::basic_string<TChar, T_char_traits>::size() - 1);
			return *this;
		}


		inline Tstr<TChar, T_char_traits> & TrimRight()
		{
			size_t i = 0;
			for (; i < std::basic_string<TChar, T_char_traits>::size(); i++) {
				if (!isspace((*this)[i])) {
					break;
				}
			}
			*this = Tstr<TChar, T_char_traits>(std::basic_string<TChar, T_char_traits>::c_str() + i);
			return *this;
		}

		inline Tstr<TChar, T_char_traits> & Trim()
		{ 
			TrimRight();
			return TrimLeft(); 
		}
			
		friend Tstr<TChar, T_char_traits> operator+(const Tstr<TChar, T_char_traits> & a, const TChar * b)
		{
			Tstr<TChar, T_char_traits> s(a);
			s += b;
			return s;
		}
		
		friend Tstr<TChar, T_char_traits> operator+(const Tstr<TChar, T_char_traits> & a, const Tstr<TChar, T_char_traits> & b)
		{
			Tstr<TChar, T_char_traits> s(a);
			s += b;
			return s;
		}

		friend Tstr<TChar, T_char_traits> operator+(const TChar * a, const Tstr<TChar, T_char_traits> & b)
		{
			Tstr<TChar, T_char_traits> s(a);
			s += b;
			return s;
		}

	};

	class char_traits_no_case : public std::char_traits<wchar_t>
	{
	public:
		static int compare(const char_type* s1,const char_type* s2, size_t n) 
		{
			return _wcsnicmp(s1, s2, n);
		}
	};
	typedef Tstr<char, std::char_traits<char> > AString;
	typedef Tstr<wchar_t, std::char_traits<wchar_t> > WString;
	typedef Tstr<wchar_t, std::char_traits<wchar_t> > String;
	class WStringNoCase : public Tstr<wchar_t, char_traits_no_case > 
	{
	public:
		inline WStringNoCase()
		{}
		inline WStringNoCase(const wchar_t * p)
			: Tstr<wchar_t, char_traits_no_case >(p)
		{
		}
		inline WStringNoCase(const WString & str)
			: Tstr<wchar_t, char_traits_no_case >(reinterpret_cast<const WStringNoCase &>(str))
		{
		}
		inline operator WString &()
		{
			return reinterpret_cast<WString &>(*this);
		}
		friend WStringNoCase operator+(const WStringNoCase & a, const wchar_t * b)
		{
			WStringNoCase s(a);
			s += b;
			return s;
		}
		
		friend WStringNoCase operator+(const WStringNoCase & a, const WStringNoCase & b)
		{
			WStringNoCase s(a);
			s += b;
			return s;
		}

		friend WStringNoCase operator+(const wchar_t * a, const WStringNoCase & b)
		{
			WStringNoCase s(a);
			s += b;
			return s;
		}
	};

	inline void ConvertString(AString &dest, const wchar_t *p, int len = -1)
	{
		if (p && *p)
		{
			if (len<0)
				len = int(wcslen(p));
			dest.resize(len, ' ');
			// we can get away with this because dest is local and can't be shared
			char *pRet = const_cast<char *>(dest.c_str());


#ifdef __GNUG__
			for (int x=0; x<len; ++x)
			{
				if (p[x]>=256)
				{
					pRet[x] = '?';
				}
				else
					pRet[x] = char(p[x]);
			}
#else
			::WideCharToMultiByte( 28591, 0, p, len, pRet, len, "?", NULL);
#endif

			pRet[len] = 0;
		}
	}

	inline void ConvertString(AString &dest, const char *p, int /*len = -1*/)
	{
		//len;
		dest = p;
	}
	inline AString ConvertToAString(const wchar_t *p)
	{
		AString dest;
		ConvertString(dest, p);
		return dest;
	}
		
// 	inline void ConvertString(WString dest, const wchar_t *p)
	inline void ConvertString(WString &dest, const wchar_t *p, int len = -1)	// JOSH 4/4/06: made dest a ref, so this function actually does something
	{
		//len;
		dest = p;
	}
// 	inline void ConvertString(WString dest, const char *p)
	inline void ConvertString(WString &dest, const char *p, int len = -1)	// JOSH 4/4/06: made dest a ref, so this function actually does something
	{
		if (p && *p)
		{
			if (len<0)
				len = unsigned(strlen(p));
			// default 8859-1 conversion
			dest.resize(len, ' ');
			// we can get away with this because dest is local and can't be shared
			wchar_t *pRet = const_cast<wchar_t *>(dest.c_str());
	//		::MultiByteToWideChar( CP_ACP, 0, p, len, pRet, len);
			for (int x=0; x<len; x++)
				pRet[x] = (unsigned char)p[x];
			pRet[len] = '\0';
		}
	}
	inline WString ConvertToWString(const char *p)
	{
		WString dest;
		ConvertString(dest, p);
		return dest;
	}
	inline WString ConvertToWString(const WString &str)
	{
		return str;
	}
	template <class TChar> class TCompare_StrCompactWhitespace
	{
	public:
		inline static int Compare(const TChar * pA, const TChar * pB)
		{
			int nRet = 0;
			bool bInQuotes = false;
			while (*pA && *pB)
			{
				TCHAR a = *pA;
				TCHAR b = *pB;

				if (!bInQuotes)
				{
					while (a && iswspace(a))
						a = *++pA;
					while (b && iswspace(b))
						b = *++pB;
				}

				if (a!=b)
				{
					nRet = a-b;;
					break;
				}
				if (a)
				{
					++pA;
					++pB;
					if (a=='"')
						bInQuotes = !bInQuotes;

				}
			}
			return nRet;
		}
		inline bool operator()(const TChar * pA, const TChar * pB) const
		{
			return Compare(pA, pB)<0;
		}
	} ;
	typedef TCompare_StrCompactWhitespace<TCHAR> Compare_StrCompactWhitespace;

	class Error
	{
		String m_strError;
	public:
		inline Error(const String &str)
			: m_strError(str)
		{
		}
		inline Error(const AString &str)
			: m_strError(ConvertToWString(str.c_str()))
		{
		}
		inline Error(const wchar_t *str)
			: m_strError(ConvertToWString(str))
		{
		}
		inline Error(const WStringNoCase &str)
			: m_strError(ConvertToWString(str.c_str()))
		{
		}

		String GetErrorDescription() const
		{
			return m_strError;
		}
	};
	///////////////////////////////////////////////////////////////////////////////
	//
	//	class ErrorUserCanceled
	//
	///////////////////////////////////////////////////////////////////////////////
	class ErrorUserCanceled : public Error
	{
	public:
		inline ErrorUserCanceled()  
			: Error("User Canceled")
		{
		}

		inline ErrorUserCanceled(const ErrorUserCanceled &e) : Error(e) { }
	}; // ErrorUserCanceled


#define HAS_BlobDataRead
	class BlobDataRead
	{
		mutable const void * m_pBlobNext;
		const void *m_pBlobEnd;
	public:
		inline BlobDataRead(const void *pBlob, size_t nSize)
			: m_pBlobNext(pBlob), m_pBlobEnd(static_cast<const char *>(pBlob)+nSize) 
		{
		}

		inline bool IsEof()
		{
			return m_pBlobNext>=m_pBlobEnd;
		}

		inline unsigned Read(void * pDest, unsigned nSize) const
		{
			if ((static_cast<const char *>(m_pBlobNext)+nSize)>m_pBlobEnd)
				throw Error(_T("Internal Error: Attempt to read past the end of a blob."));

			memcpy(pDest, m_pBlobNext, nSize);
			m_pBlobNext = static_cast<const char *>(m_pBlobNext) + nSize;

			return nSize;
		}

		inline const void * Get(unsigned nSize) const
		{
			if ((static_cast<const char *>(m_pBlobNext)+nSize)>m_pBlobEnd)
				throw Error(_T("Internal Error: Attempt to read past the end of a blob."));

			const void * pRet = m_pBlobNext;
			m_pBlobNext = static_cast<const char *>(m_pBlobNext) + nSize;
			return pRet;
		}
	};

	namespace SHPBlob
	{
		template <class TFile> bool ValidateShpBlob(TFile &file, bool bFileMode = false)
		{
			return false;
		}
	};

	template <class TDataType> class SmartPointerRefObj
	{
		TDataType *pData;

		inline void DereferenceObj()
		{
			if (pData)
			{
#ifdef __GNUG__
				//__sync_fetch_and_add(&pData->m_SmartPointerRefObj_refCount, -1);
				//@todo: Check this!
				(pData->m_SmartPointerRefObj_refCount)--;
				if((&pData->m_SmartPointerRefObj_refCount)==0)
#else
				if (InterlockedDecrement(&pData->m_SmartPointerRefObj_refCount)==0)
#endif
				{
					delete pData;
					pData = NULL;
				}
			}
		}
		inline void ReferenceObj()
		{
			if (pData)
#ifdef __GNUG__
				//__sync_fetch_and_add(&pData->m_SmartPointerRefObj_refCount, 1);
				//@todo: Check this!
				(pData->m_SmartPointerRefObj_refCount)++;
#else
				InterlockedIncrement(&pData->m_SmartPointerRefObj_refCount);
#endif
		}

	public:
	///////////////////////////////////////////////////////////////////////////////
	// Function name	: SmartPointerRefObj
	// Description:   This version of the consructor will own the pointer that is passed in
	//					DO NOT delete the pointer yourself.  Make sure the pointer is to dynamicly allocated data
	// Return "inline": 
	// Arguments:
	//    const TDataType * o: 
	///////////////////////////////////////////////////////////////////////////////
		inline SmartPointerRefObj(TDataType * o=NULL)
			: pData(o)
		{
			ReferenceObj();
		}

		inline SmartPointerRefObj(const SmartPointerRefObj<TDataType> &o)
		{
			if (!o.pData)
				pData = NULL;
			else
			{
				pData = o.pData;
				ReferenceObj();
			}
		}

		// KLUDGE: this says it takes a const, but the only thing that is const is the pointer.
		// the thing the pointer points to can change
		inline SmartPointerRefObj<TDataType> & operator =(const SmartPointerRefObj<TDataType> &o)
		{
			DereferenceObj();
			if (!o.pData)
				pData = NULL;
			else
			{
				pData = o.pData;
				ReferenceObj();
			}
			return *this;
		}

		inline SmartPointerRefObj<TDataType> & operator =(TDataType *pO)
		{
			DereferenceObj();

			pData = pO;
			ReferenceObj();

			return *this;
		}

		//////////////////////////////////////////////////////////////////
		// Note: the three comparison operators defined here compare the pointers,
		// not the data pointed to.  Thus pointers to two copies of exactly the same 
		// data will not be equal.
		//////////////////////////////////////////////////////////////////
		inline bool operator ==(const SmartPointerRefObj<TDataType> & o) const
		{
			return pData==o.pData;
		}

		inline bool operator !=(const SmartPointerRefObj<TDataType> & o) const
		{
			return pData!=o.pData;
		}

		inline bool operator <(const SmartPointerRefObj<TDataType> & o) const // RB 1/6/00
		{
			return pData < o.pData;
		}

		inline ~SmartPointerRefObj()
		{
			DereferenceObj();
		}

		inline const TDataType * operator ->() const
		{
			assert(pData);
			return pData;
		}

		inline TDataType * operator ->()
		{
			assert(pData);
			return pData;
		}

		inline const TDataType & operator *() const
		{
			assert(pData);
			return *(pData);
		}

		inline TDataType & operator *()
		{
			assert(pData);
			return *(pData);
		}

		inline void Delete()
		{
			DereferenceObj();
			pData = NULL;
		}

		inline TDataType *Get()
		{
			return pData ? pData : NULL;
		}

		inline const TDataType *Get() const
		{
			return pData ? pData : NULL;
		}
		template <class TDestDataType> SmartPointerRefObj<TDestDataType> StaticCast()
		{
			// guard against multiple inheritance...
			// it will also force the compiler to check if this cast is valid
			if (static_cast<TDestDataType *>(pData)!=pData)
				throw Error("SmartPointer Static cast is not valid for multiple inheritance.");

			return SmartPointerRefObj<TDestDataType>(reinterpret_cast<SmartPointerRefObj<TDestDataType> &>(*this));
		}

	}; // SmartPointerRefObj
	///////////////////////////////////////////////////////////////////////////////
	// class SmartPointerRefObj
	//
	// This is similar to the RefCountObj, but without the locking and unlocking,
	// It also will never create a new obj.
	///////////////////////////////////////////////////////////////////////////////
	struct SmartPointerRefObj_Base
	{
		template<class T> friend class SmartPointerRefObj;
		long m_SmartPointerRefObj_refCount;
	protected:
		inline SmartPointerRefObj_Base()
			: m_SmartPointerRefObj_refCount(0)
		{
		}
		inline ~SmartPointerRefObj_Base()
		{
		}
		inline SmartPointerRefObj_Base( const SmartPointerRefObj_Base& )
			: m_SmartPointerRefObj_refCount(0)
		{
		}
		SmartPointerRefObj_Base& operator=( const SmartPointerRefObj_Base& )
		{
			return *this;
		}
	};
}