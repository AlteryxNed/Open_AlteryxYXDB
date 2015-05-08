///////////////////////////////////////////////////////////////////////////////
//
// (C) 2005 SRC, LLC  -   All rights reserved
//
///////////////////////////////////////////////////////////////////////////////
//
// Module: FIELDTYPES.H
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#include "Record.h"
#include <limits>
#include <algorithm>
#ifndef SRCLIB_REPLACEMENT
#include "..\..\inc\GeoJson.h"
#endif

#include <cmath>
#include <float.h>

#ifndef __GNUG__
#pragma warning(push)
#pragma warning(disable : 4127)  // conditional expression is constant
#else
#define _isnan __isnan
#endif

// windows.h defines these and they cause issues
#undef min
#undef max


namespace SRC
{
	///////////////////////////////////////////////////////////////////////////////
	// class Field_Bool
	class Field_Bool : public FieldBase
	{
	public :
		inline static bool TestChar(wchar_t c)
		{
			return (c!='0' && iswdigit(c)) || 'T'==towupper(c);
		}
		template <class TNumber> inline static bool ConvertNumber(TNumber n)
		{
			return n!=0;
		}
	private:
		TFieldVal<bool> GetVal(const RecordData * pRecord) const;
		void SetVal(Record *pRecord, bool val) const;
	public:
		inline Field_Bool(WStringNoCase strFieldName)
			: FieldBase(strFieldName, E_FT_Bool, 1, false, 1, 0)
		{}
		virtual SmartPointerRefObj<FieldBase> Copy() const
		{
			return CopyHelper(new Field_Bool(GetFieldName()));
		}

		virtual TFieldVal<bool> GetAsBool(const RecordData * pRecord) const ;
		virtual TFieldVal<int> GetAsInt32(const RecordData * pRecord) const ;
		virtual TFieldVal<AStringVal > GetAsAString(const RecordData * pRecord) const;
		virtual TFieldVal<WStringVal > GetAsWString(const RecordData * pRecord) const;
		
		virtual void SetFromBool(Record *pRecord, bool bVal) const;
		virtual void SetFromInt32(Record *pRecord, int nVal) const;
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const;
		virtual void SetFromDouble(Record *pRecord, double dVal) const;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const;
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const;

		virtual bool GetNull(const RecordData * pRecord) const;
		virtual void SetNull(Record *pRecord) const;

	};


	// the 32bit compiler likes to optimize away this test
#ifndef __GNUG__
#pragma optimize( "", off )
#endif
	// returns true if the double can NOT fit the int64
	
	template <class TFloat, class TInt> inline bool TestIntToFloat(TInt )
	{
		return false;
	}
	template <> inline bool TestIntToFloat<float, __int64>(__int64 x)
	{
		return x>(1ll<<(FLT_MANT_DIG-1)) || x<-(1ll<<(FLT_MANT_DIG-1));
	}
	template <> inline bool TestIntToFloat<double, __int64>(__int64 x)
	{
		return x>(1ll<<(DBL_MANT_DIG-1)) || x<-(1ll<<(DBL_MANT_DIG-1));
	}

	template <> inline bool TestIntToFloat<double, int>(int )
	{
		return false;
	}
	template <> inline bool TestIntToFloat<float, int>(int x)
	{
		return x>(1<<(FLT_MANT_DIG-1)) || x<-(1<<(FLT_MANT_DIG-1));
	}

#ifndef __GNUG__
#pragma optimize( "", on )
#endif

	template <class TChar> inline void ForceNullTerminated(Tstr<TChar> & buffer, const TChar *&pVal, size_t nLen)
	{
		if (pVal[nLen]!=0)
		{
			buffer.Truncate(0);
			buffer.Append(pVal, unsigned(nLen));
			pVal = buffer;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// class Field_Num
	template <int ft, class T_Num> class Field_Num : public FieldBase
	{
	protected:
		inline TFieldVal<T_Num> GetVal(const RecordData * pRecord) const
		{
			if (*(ToCharP(pRecord) + GetOffset() + sizeof(T_Num)))
				return TFieldVal<T_Num>(true, 0);

			return TFieldVal<T_Num>(false, *reinterpret_cast<const T_Num *>(ToCharP(pRecord) + GetOffset()));
		}

		inline void SetVal(Record *pRecord, T_Num val) const
		{
			memcpy(ToCharP(pRecord->GetRecord()) + GetOffset(), &val, sizeof(T_Num));

			// and set the NULL flag
			*(ToCharP(pRecord->GetRecord()) + GetOffset() + sizeof(T_Num)) = 0;
		}

		template<class TNum> inline void NumericConversionError(TNum n, const wchar_t *pDestType = NULL) const
		{
			if (m_pGenericEngine)
			{
				WString strError = WString().Assign(n);
				strError += L" does not fit in the type ";
				if (pDestType==NULL)
					strError += GetNameFromFieldType(m_ft);
				else 
					strError += pDestType;

				ReportFieldConversionError(strError);
			}
		}

		template<class TChar> inline void CheckStringConvError(Record *pRecord, const TChar *pVal, size_t nLen, const TChar *pEnd) const
		{
			if (errno==ERANGE)
			{
				SetNull(pRecord);
				if (m_pGenericEngine) // this gets set to NULL when the limit has been hit, so no point in adding the strings in the error message
				{
					if (sizeof(T_Num)==8)
						ReportFieldConversionError(ConvertToWString(pVal) + L" does not fit in an Int64.");
					else
						ReportFieldConversionError(ConvertToWString(pVal) + L" does not fit in an Int32.");
				}
			}

			if (pEnd!=(pVal+nLen) || nLen==0)
			{
				if (pEnd==pVal || nLen==0)
				{
					SetNull(pRecord);
					if (nLen>0 && m_pGenericEngine)
						ReportFieldConversionError(ConvertToWString(pVal) + L" is not a number.");
				}
				else if (*pEnd==',')
				{
					if (m_pGenericEngine)
						ReportFieldConversionError(ConvertToWString(pVal) + L" stopped converting at a comma. It might be invalid.");
				}
				else
				{
					if (m_pGenericEngine)
						ReportFieldConversionError(ConvertToWString(pVal) + L" was not fully converted");
				}
			}
		}
	public:
		inline Field_Num(WString strFieldName, int nScale=-1)
			: FieldBase(strFieldName, static_cast<E_FieldType>(ft), sizeof(T_Num)+1, false, sizeof(T_Num), nScale)
		{}

		virtual SmartPointerRefObj<FieldBase> Copy() const
		{
			return CopyHelper(new Field_Num<ft, T_Num>(GetFieldName(), m_nScale));
		}


		virtual TFieldVal<bool> GetAsBool(const RecordData * pRecord) const 
		{
			TFieldVal<T_Num> val = GetVal(pRecord);
			TFieldVal<bool> ret(val.bIsNull, false);
			if (!val.bIsNull)
				ret.value = Field_Bool::ConvertNumber(val.value);

			return ret;
		}
		virtual TFieldVal<int> GetAsInt32(const RecordData * pRecord) const 
		{
			TFieldVal<T_Num> val = GetVal(pRecord);
			if ((!std::numeric_limits<T_Num>::is_integer || sizeof(T_Num)>sizeof(int)) && 
				(val.value>T_Num(std::numeric_limits<int>::max()) || val.value<T_Num(std::numeric_limits<int>::min())))
			{
				NumericConversionError(val.value, L"Int32");
				val.bIsNull = true;
				val.value = 0;
			}
			if (!std::numeric_limits<T_Num>::is_integer)
				return TFieldVal<int>(val.bIsNull, static_cast<int>(val.value+(val.value<0 ? -0.5 : 0.5)));
			else
				return TFieldVal<int>(val.bIsNull, static_cast<int>(val.value));
		}
		virtual TFieldVal<__int64> GetAsInt64(const RecordData * pRecord) const 
		{
			TFieldVal<T_Num> val = GetVal(pRecord);
			if (!std::numeric_limits<T_Num>::is_integer  && 
				(val.value>std::numeric_limits<__int64>::max() || val.value<std::numeric_limits<__int64>::min()))
			{
				NumericConversionError(val.value , L"Int64");
				val.bIsNull = true;
				val.value = 0;
			}
			if (!std::numeric_limits<T_Num>::is_integer)
				return TFieldVal<__int64>(val.bIsNull, static_cast<__int64>(val.value+(val.value<0 ? -0.5 : 0.5)));
			else
				return TFieldVal<__int64>(val.bIsNull, static_cast<__int64>(val.value));
		}
		virtual TFieldVal<double> GetAsDouble(const RecordData * pRecord) const 
		{
			TFieldVal<T_Num> val = GetVal(pRecord);
			if (std::numeric_limits<T_Num>::is_integer && TestIntToFloat<double>(val.value))
				NumericConversionError(val.value , L"Double");
			return TFieldVal<double>(val.bIsNull, static_cast<double>(val.value));
		}

		virtual TFieldVal<AStringVal > GetAsAString(const RecordData * pRecord) const 
		{
			TFieldVal<T_Num> val = GetVal(pRecord);
			if (val.bIsNull)
				m_astrTemp.Truncate(0);
			else
				m_astrTemp.Assign(val.value);
			TFieldVal<AStringVal > ret(val.bIsNull, AStringVal(m_astrTemp.Length(), m_astrTemp.c_str()));
			return ret;
		}
		virtual TFieldVal<WStringVal > GetAsWString(const RecordData * pRecord) const 
		{
			TFieldVal<T_Num> val = GetVal(pRecord);
			if (val.bIsNull)
				m_wstrTemp.Truncate(0);
			else
				m_wstrTemp.Assign(val.value);
			TFieldVal<WStringVal > ret(val.bIsNull, WStringVal(m_wstrTemp.Length(), m_wstrTemp.c_str()));
			return ret;
		}
	
		virtual void SetFromInt32(Record *pRecord, int nVal) const
		{
			if (sizeof(T_Num)<sizeof(int) && 
				(nVal>std::numeric_limits<T_Num>::max() || nVal<std::numeric_limits<T_Num>::min()))
			{
				NumericConversionError(nVal);
				SetNull(pRecord);
			}
			else
			{
				if (!std::numeric_limits<T_Num>::is_integer && TestIntToFloat<T_Num>(nVal))
					NumericConversionError(nVal);
				SetVal(pRecord, static_cast<T_Num>(nVal));
			}
		}
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const
		{
			if (sizeof(T_Num)<sizeof(__int64) && std::numeric_limits<T_Num>::is_integer && 
				(nVal>std::numeric_limits<T_Num>::max() || nVal<std::numeric_limits<T_Num>::min()))
			{
				NumericConversionError(nVal);
				SetNull(pRecord);
			}
			else
			{
				if (!std::numeric_limits<T_Num>::is_integer && TestIntToFloat<T_Num>(nVal))
					NumericConversionError(nVal);

				SetVal(pRecord, static_cast<T_Num>(nVal));
			}
		}
		virtual void SetFromDouble(Record *pRecord, double dVal) const
		{
			if (std::numeric_limits<T_Num>::is_integer && 
				(dVal>std::numeric_limits<T_Num>::max() || dVal<std::numeric_limits<T_Num>::min()))
			{
				NumericConversionError(dVal);
				SetNull(pRecord);
			}
			else
			{
				if (std::numeric_limits<T_Num>::is_integer)
					dVal += dVal<0 ? -0.5 : 0.5;
				SetVal(pRecord, static_cast<T_Num>(dVal));
			}
		}
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const
		{
			ForceNullTerminated<char>(m_astrTemp, pVal, nLen);

			errno = 0;
			char * pEnd;
			if (sizeof(T_Num)==8)
				SetVal(pRecord, static_cast<T_Num>(_strtoi64(pVal, &pEnd, 10)));
			else
				SetVal(pRecord, static_cast<T_Num>(strtol(pVal, &pEnd, 10)));

			CheckStringConvError(pRecord, pVal, nLen, pEnd);
		}

		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const
		{
			ForceNullTerminated<wchar_t>(m_wstrTemp, pVal, nLen);

			errno = 0;
			wchar_t * pEnd;
			if (sizeof(T_Num)==8)
				SetVal(pRecord, static_cast<T_Num>(_wcstoi64(pVal, &pEnd, 10)));
			else
				SetVal(pRecord, static_cast<T_Num>(wcstol(pVal, &pEnd, 10)));

			CheckStringConvError(pRecord, pVal, nLen, pEnd);
		}

		virtual bool GetNull(const RecordData * pRecord) const
		{
			return *(ToCharP(pRecord) + GetOffset() + sizeof(T_Num)) != 0;
		}

		virtual void SetNull(Record *pRecord) const
		{
			memset(ToCharP(pRecord->GetRecord()) + GetOffset(), 0, sizeof(T_Num));

			// and set the NULL flag
			*(ToCharP(pRecord->GetRecord()) + GetOffset() + sizeof(T_Num)) = 1;
		}
	};

	typedef Field_Num<E_FT_Byte, unsigned char> Field_Byte;
	typedef Field_Num<E_FT_Int16, signed short> Field_Int16;
	typedef Field_Num<E_FT_Int32, signed int> Field_Int32;
	typedef Field_Num<E_FT_Int64, signed __int64> Field_Int64;
	
	///////////////////////////////////////////////////////////////////////////////
	// class Field_Float
	template <int ft, class T_Num> class T_Field_Float : public Field_Num<ft, T_Num>
	{
		template <class TChar> inline void TSetFromString(Record *pRecord, const TChar * pVal, size_t nLen) const
		{
			if (nLen==0)
			{
				Field_Num<ft, T_Num>::SetNull(pRecord);
			}
			else
			{
				double dVal;
				bool bOverflow = false;
				unsigned nNumCharsUsed = ConvertToDouble(pVal, dVal, bOverflow);
				if (0==nNumCharsUsed || _isnan(dVal))
				{
					if (Field_Num<ft, T_Num>::IsReportingFieldConversionErrors())
						ReportFieldConversionError(ConvertToWString(pVal) + L" is not a valid number.");
					Field_Num<ft, T_Num>::SetNull(pRecord);
				}
				else
				{
					if (Field_Num<ft, T_Num>::IsReportingFieldConversionErrors())
					{
						if (bOverflow)
							ReportFieldConversionError(ConvertToWString(pVal) + L" had more precision than a double. Some precision was lost.");
						else if (nNumCharsUsed!=nLen && Field_Num<ft, T_Num>::m_pGenericEngine)
						{
							if (pVal[nNumCharsUsed]==',')
								ReportFieldConversionError(ConvertToWString(pVal) + L" stopped converting at a comma. It might be invalid.");
							else
								ReportFieldConversionError(ConvertToWString(pVal) + L" lost information in translation");
						}
					}

					SetVal(pRecord, T_Num(dVal));
				}
			}
		}

	public:
		inline T_Field_Float(WString strFieldName, int nScale = -1)
			: Field_Num<ft, T_Num>(strFieldName, nScale)
		{}
		virtual SmartPointerRefObj<FieldBase> Copy() const
		{
			return Field_Num<ft, T_Num>::CopyHelper(new T_Field_Float<ft, T_Num>(Field_Num<ft, T_Num>::GetFieldName(), Field_Num<ft, T_Num>::m_nScale));
		}
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const
		{
			// FieldBase:: is added explicit for g++ compiler does not find it otherwise
			ForceNullTerminated<char>(this->m_astrTemp, pVal, nLen);
			return TSetFromString(pRecord, pVal, nLen);
		}
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const
		{
			// FieldBase:: is added explicit for g++ compiler does not find it otherwise
			ForceNullTerminated<wchar_t>(this->m_wstrTemp, pVal, nLen);
			return TSetFromString(pRecord, pVal, nLen);
		}
	};

	typedef T_Field_Float<E_FT_Float, float> Field_Float;
	typedef T_Field_Float<E_FT_Double, double> Field_Double;

	///////////////////////////////////////////////////////////////////////////////
	// class Field_String_Store
	template <class TChar> class Field_String_GetSet_Buffer
	{
	protected:
		mutable TChar *m_pBuffer;
		mutable unsigned m_nBufferSize;

		inline void Allocate(unsigned nSize) const
		{
			// round up to the nearest power of 2.
			unsigned alloc_size = 256;
			while (alloc_size<nSize)
				alloc_size <<= 1;

			if (m_pBuffer!=NULL)
				delete [] m_pBuffer;

			m_pBuffer = new TChar[nSize];
			m_nBufferSize = nSize;
		}
	protected:

		inline Field_String_GetSet_Buffer()
			: m_pBuffer(NULL)
			, m_nBufferSize(0)
		{
		}
		inline ~Field_String_GetSet_Buffer()
		{
			if (m_pBuffer)
				delete [] m_pBuffer;
		}
	};

	template <class TChar> class Field_String_GetSet : public Field_String_GetSet_Buffer<TChar>
	{
	public:
		inline static TFieldVal<char> GetVal1stChar(const RecordData * pRecord, int nOffset, int nFieldLen)
		{
			TFieldVal<char> ret(false, 0);

			ret.bIsNull = 0 != *(ToCharP(pRecord) + nOffset + nFieldLen*sizeof(TChar));
			if (!ret.bIsNull)
				ret.value = *(ToCharP(pRecord) + nOffset);

			return ret;
		}
		inline TFieldVal<TBlobVal<TChar> > GetVal(const RecordData * pRecord, unsigned nOffset, unsigned nFieldLen) const
		{
			TFieldVal<TBlobVal<TChar> > ret;
			ret.bIsNull = 0!=*(ToCharP(pRecord) + nOffset + nFieldLen*sizeof(TChar));
			if (!ret.bIsNull)
			{
				if (sizeof(TChar)==2)
				{
					if (Field_String_GetSet_Buffer<TChar>::m_nBufferSize<=nFieldLen)
						Field_String_GetSet_Buffer<TChar>::Allocate(nFieldLen+1);
					memcpy(Field_String_GetSet_Buffer<TChar>::m_pBuffer, ToCharP(pRecord) + nOffset, nFieldLen*sizeof(TChar));
					Field_String_GetSet_Buffer<TChar>::m_pBuffer[nFieldLen] = 0;
					ret.value.pValue = Field_String_GetSet_Buffer<TChar>::m_pBuffer;
				}
				else
				{
					// we don't need to copy the buffer, it should be NULL terminated already.
					// since the NULL flag is 0 if it is NOT NULL and follows the string
					ret.value.pValue = reinterpret_cast<const TChar *>(ToCharP(pRecord) + nOffset); 
				}

				const TChar *p;
				for (p = ret.value.pValue; *p; ++p)
					;
				ret.value.nLength = unsigned(p-ret.value.pValue);
			}

			return ret;
		}

		inline static void SetVal(const FieldBase *pField, Record *pRecord, int nOffset, size_t nFieldLen, const TChar *pVal, size_t nLen)
		{
			if (nLen>nFieldLen && pField->GetGenericEngine())
				pField->ReportFieldConversionError(L"\"" + ConvertToWString(pVal) + L"\" was truncated");

			char *pFieldData = ToCharP(pRecord->GetRecord()) + nOffset;
			// reset the Null flag
			// since the NULL flag is 0 if it is NOT NULL and follows the string
			// we always have a NULL terminated string
			*(pFieldData + nFieldLen*sizeof(TChar)) = 0;

			memcpy(pFieldData, pVal, std::min(nFieldLen, nLen)*sizeof(TChar) );

			// the input is not always null terminated
			if (nLen<nFieldLen)
				reinterpret_cast<TChar *>(pFieldData)[nLen] = 0;

		}
		inline static bool GetNull(const RecordData * pRecord, int nOffset, int nFieldLen)
		{
			return *(ToCharP(pRecord) + nOffset + nFieldLen*sizeof(TChar)) != 0;
		}
		inline static void SetNull(Record *pRecord, int nOffset, int nFieldLen)
		{
			*(ToCharP(pRecord->GetRecord()) + nOffset + nFieldLen*sizeof(TChar)) = 1;
		}
		inline static TFieldVal<BlobVal > GetAsBlob(const RecordData * pRecord, int nOffset, int nFieldLen)
		{
			TFieldVal<BlobVal> ret(false, BlobVal(0, NULL));
			ret.bIsNull = 0!=*(ToCharP(pRecord) + nOffset + nFieldLen*sizeof(TChar));
			if (!ret.bIsNull)
			{
				ret.value.pValue = (ToCharP(pRecord) + nOffset);
				ret.value.nLength = nFieldLen*sizeof(TChar);
			}
			return ret;
		}
	};
	///////////////////////////////////////////////////////////////////////////////
	// class Field_V_String_Store
	template <class TChar> class Field_V_String_GetSet : public Field_String_GetSet_Buffer<TChar>
	{
	public:
		inline static TFieldVal<char> GetVal1stChar(const RecordData * pRecord, int nOffset, int /*nFieldLen*/)
		{
			BlobVal val = RecordInfo::GetVarDataValue(pRecord, nOffset);
			TFieldVal<char> ret;

			// Code added by Josh 4/7/06 to fix conversion from String to Bool.
			// Previous code returned (first == true) if second held a valid char*
			// but this is inconsistent with all the other pair<bool,...> objects,
			// which imply that if (first == true), then second is invalid.
			// I corrected this so that first == true iff the char* is NULL.
			ret.bIsNull = val.pValue == NULL;
			if (!ret.bIsNull)
				ret.value = *static_cast<const char *>(val.pValue);
			return ret;
		}
		inline TFieldVal<TBlobVal<TChar> > GetVal(const RecordData * pRecord, unsigned nOffset, unsigned nFieldLen) const
		{
			BlobVal val = RecordInfo::GetVarDataValue(pRecord, nOffset);
			TFieldVal<TBlobVal<TChar> > ret;
			ret.bIsNull = val.pValue==NULL;
			if (!ret.bIsNull)
			{
				ret.value.nLength = val.nLength/sizeof(TChar);

				assert(ret.value.nLength<=nFieldLen);
#ifndef __GNUG__
				nFieldLen; // remove warning in release
#endif

				if (Field_String_GetSet_Buffer<TChar>::m_nBufferSize<=ret.value.nLength)
					Allocate(ret.value.nLength+1);
				memcpy(Field_String_GetSet_Buffer<TChar>::m_pBuffer, static_cast<const char *>(val.pValue), val.nLength);
				Field_String_GetSet_Buffer<TChar>::m_pBuffer[ret.value.nLength] = 0;
				ret.value.pValue = Field_String_GetSet_Buffer<TChar>::m_pBuffer;
			}

			return ret;
		}

		inline static void SetVal(const FieldBase *pField, Record *pRecord, int nOffset, size_t nFieldLen, const TChar *pVal, size_t nLen)
		{
			if (nLen>nFieldLen && pField->GetGenericEngine())
			{
				if (nFieldLen>100)
				{
					TChar buffer[101];
					memcpy(buffer, pVal, 97);
					buffer[97]='.';
					buffer[98]='.';
					buffer[99]='.';
					buffer[100]=0;
					pField->ReportFieldConversionError(WString(L"\"") + ConvertToWString(buffer) + L"\" was truncated");
				}
				else
					pField->ReportFieldConversionError(WString(L"\"") + ConvertToWString(pVal) + L"\" was truncated");
			}

			RecordInfo::SetVarDataValue(pRecord, nOffset, unsigned(std::min(nFieldLen, nLen)*sizeof(TChar)), pVal);
		}
		inline static bool GetNull(const RecordData * pRecord, int nOffset, int /*nFieldLen*/)
		{
			return RecordInfo::GetVarDataValue(pRecord, nOffset).pValue==NULL;
		}

		inline static void SetNull(Record *pRecord, int nOffset, int /*nFieldLen*/)
		{
			RecordInfo::SetVarDataValue(pRecord, nOffset, 0, NULL);
		}
		inline static TFieldVal<BlobVal> GetAsBlob(const RecordData * pRecord, int nOffset, int /*nFieldLen*/)
		{
			TFieldVal<BlobVal> ret(false, RecordInfo::GetVarDataValue(pRecord, nOffset));
			ret.bIsNull = ret.value.pValue==NULL;
			return ret;
		}
	};

	///////////////////////////////////////////////////////////////////////////////
	// class Field_String
	template <E_FieldType ft, class TChar, class TStorage, bool bIsVarData> class T_Field_String : public FieldBase
	{
		// a temporary spot for holding a spatial object after converting from gjson
		mutable unsigned char * m_pSpatialObj;

	protected:
		mutable TStorage m_storage;
		mutable Tstr<TChar> m_strBuffer;
		
		//returns true if it should be NULL
		template<class TCharInner> inline bool CheckNumberConvError(const TCharInner *pVal, size_t nLen, const TCharInner *pEnd, bool bInt64) const
		{
			if (errno==ERANGE)
			{
				if (bInt64)
					ReportFieldConversionError(ConvertToWString(pVal) + L" does not fit in an Int64.");
				else
					ReportFieldConversionError(ConvertToWString(pVal) + L" does not fit in an Int32.");
				return true;
			}

			if (pEnd!=(pVal+nLen) || nLen==0)
			{
				if (pEnd==pVal || nLen==0)
				{
					if (nLen>0 && m_pGenericEngine)
						ReportFieldConversionError(ConvertToWString(pVal) + L" is not a number.");
					return true;
				}
				else if (*pEnd==',')
				{
					if (m_pGenericEngine)
						ReportFieldConversionError(ConvertToWString(pVal) + L" stopped converting at a comma. It might be invalid.");
				}
				else
				{
					if (m_pGenericEngine)
						ReportFieldConversionError(ConvertToWString(pVal) + L" lost information in translation");
				}
			}
			return false;
		}
		inline void DoConvertString(WString &strBuffer, const wchar_t * pVal, size_t /*nLen*/) const
		{
			// this should not ever be called.  It is only implemented because of template stuff
			assert(false);
			strBuffer = pVal;
		}
		inline void DoConvertString(AString &strBuffer, const wchar_t * pVal, size_t nLen) const
		{
			if (nLen<0)
				nLen = unsigned(wcslen(pVal));

			unsigned nNarrowLen = unsigned(nLen+1);
			char *pRet = strBuffer.Lock( nNarrowLen );
			int bConversionError = false;
#ifdef __GNUG__
			for (unsigned x=0; x<nLen; ++x)
			{
				if (pVal[x]>=256)
				{
					pRet[x] = '?';
					bConversionError = true;
				}
				else
					pRet[x] = char(pVal[x]);
			}
#else
			//28591== ISO 8859-1 Latin I 
			::WideCharToMultiByte( 28591, m_pGenericEngine!=nullptr ? WC_NO_BEST_FIT_CHARS : 0, pVal, unsigned(nLen), pRet, nNarrowLen, "?", &bConversionError );
#endif
			pRet[nLen] = 0;
			strBuffer.Unlock();
			if (m_pGenericEngine && bConversionError)
			{
				// do the conversion again, this time allowing best fit characters.
				// we wanted to generate a Conv Error if anything changed, but now we want to get the 
				// best guess to the user
				pRet = strBuffer.Lock( nNarrowLen );

#ifndef __GNUG__
				::WideCharToMultiByte( 28591, 0, pVal, unsigned(nLen), pRet, nNarrowLen, "?", &bConversionError );
				pRet[nLen] = 0;
				strBuffer.Unlock();
#endif

				ReportFieldConversionError(L"\"" + WString(pVal) + L"\" could not be fully converted from a WString to a String.");
			}
		}

	public:
		inline T_Field_String(WStringNoCase strFieldName, int nFieldSize, int nScale=-1, E_FieldType nFieldType=ft)
			: FieldBase(strFieldName, nFieldType, bIsVarData ? 4 : nFieldSize*sizeof(TChar)+1, bIsVarData, nFieldSize, nScale)
			, m_pSpatialObj(NULL)
		{
		}
		virtual ~T_Field_String()
		{
			if (m_pSpatialObj)
				free(m_pSpatialObj);
		}
		virtual SmartPointerRefObj<FieldBase> Copy() const
		{
			return CopyHelper(new T_Field_String<ft, TChar, TStorage, bIsVarData>(GetFieldName(), m_nSize, m_nScale, m_ft));
		}

	public:
		/*virtual*/ unsigned GetMaxBytes() const
		{
			return m_nSize * sizeof(TChar);
		}

		virtual TFieldVal<bool> GetAsBool(const RecordData * pRecord) const 
		{
			TFieldVal<char> val = TStorage::GetVal1stChar(pRecord, GetOffset(), m_nSize);
			TFieldVal<bool> ret(val.bIsNull, false);
			if (!ret.bIsNull)
				ret.value = Field_Bool::TestChar(val.value);
			return ret;
		}
		virtual TFieldVal<int> GetAsInt32(const RecordData * pRecord) const 
		{
			TFieldVal<TBlobVal<TChar> > val = m_storage.GetVal(pRecord, GetOffset(), m_nSize);
			TFieldVal<int> ret(val.bIsNull, 0);
			if (!ret.bIsNull)
			{
				errno = 0;
				if (sizeof(TChar)==1)
				{
					const char * pBegin = (const char *)val.value.pValue;
					char * pEnd;
					ret.value = strtol(pBegin, &pEnd, 10);
					ret.bIsNull =  CheckNumberConvError(pBegin, val.value.nLength, pEnd, false);
				}
				else
				{
					const wchar_t * pBegin = (const wchar_t *)val.value.pValue;
					wchar_t * pEnd;
					ret.value = wcstol(pBegin, &pEnd, 10);
					ret.bIsNull =  CheckNumberConvError(pBegin, val.value.nLength, pEnd, false);
				}
			}
			return ret;
		}
		virtual TFieldVal<__int64> GetAsInt64(const RecordData * pRecord) const 
		{
			TFieldVal<TBlobVal<TChar> > val(m_storage.GetVal(pRecord, GetOffset(), m_nSize));
			TFieldVal<__int64> ret(val.bIsNull, 0);
			if (!ret.bIsNull)
			{
				if (sizeof(TChar)==1)
				{
					const char * pBegin = (const char *)val.value.pValue;
					char * pEnd;
					errno = 0;
					ret.value = _strtoi64(pBegin, &pEnd, 10);
					ret.bIsNull =  CheckNumberConvError(pBegin, val.value.nLength, pEnd, true);
				}
				else
				{
					const wchar_t * pBegin = (const wchar_t *)val.value.pValue;
					wchar_t * pEnd;
					errno = 0;
					ret.value = _wcstoi64(pBegin, &pEnd, 10);
					ret.bIsNull =  CheckNumberConvError(pBegin, val.value.nLength, pEnd, true);
				}
			}
			return ret;
		}
		virtual TFieldVal<double> GetAsDouble(const RecordData * pRecord) const 
		{
			TFieldVal<TBlobVal<TChar> > val(m_storage.GetVal(pRecord, GetOffset(), m_nSize));
			TFieldVal<double> ret(val.bIsNull, 0.0);
			unsigned nLen = val.value.nLength;
			if (nLen==0)
				ret.bIsNull = true;
			if (!ret.bIsNull)
			{
				bool bOverflow = false;
				unsigned nNumCharsUsed = ConvertToDouble(val.value.pValue, ret.value, bOverflow);
				if (0==nNumCharsUsed || _isnan(ret.value))
				{
					if (IsReportingFieldConversionErrors())
						ReportFieldConversionError(ConvertToWString(val.value.pValue) + L" is not a valid number.");
					ret.bIsNull = true;
					ret.value = 0.0;
				}
				else
				{
					if (IsReportingFieldConversionErrors())
					{
						if (bOverflow)
							ReportFieldConversionError(ConvertToWString(val.value.pValue) + L" had more precision than a double. Some precision was lost.");
						else if (nNumCharsUsed!=nLen && m_pGenericEngine)
						{
							if (val.value.pValue[nNumCharsUsed]==',')
								ReportFieldConversionError(ConvertToWString(val.value.pValue) + L" stopped converting at a comma. It might be invalid.");
							else
								ReportFieldConversionError(ConvertToWString(val.value.pValue) + L" lost information in translation");
						}
					}
				}
			}

			return ret;
		}
		inline TFieldVal<AStringVal > GetAsAString(const TFieldVal<AStringVal > & val) const
		{
			return val;
		}
		inline TFieldVal<AStringVal > GetAsAString(const TFieldVal<WStringVal > & val) const
		{
			TFieldVal<AStringVal > ret;

			ret.bIsNull = val.bIsNull;
			DoConvertString(m_astrTemp, (const wchar_t *)val.value.pValue, val.value.nLength);
			ret.value = AStringVal(m_astrTemp.Length(), m_astrTemp.c_str());
			return ret;
		}
		virtual TFieldVal<AStringVal > GetAsAString(const RecordData * pRecord) const
		{
			return GetAsAString(m_storage.GetVal(pRecord, GetOffset(), m_nSize));
		}

		inline TFieldVal<WStringVal > GetAsWString(const TFieldVal<WStringVal > & val) const
		{
			return val;
		}
		inline TFieldVal<WStringVal > GetAsWString(const TFieldVal<AStringVal > & val) const
		{
			TFieldVal<WStringVal > ret;

			ret.bIsNull = val.bIsNull;
			m_wstrTemp = ConvertToWString(val.value.pValue);
			ret.value = WStringVal(m_wstrTemp.Length(), m_wstrTemp.c_str());
			return ret;
		}
		virtual TFieldVal<WStringVal > GetAsWString(const RecordData * pRecord) const
		{
			return GetAsWString(m_storage.GetVal(pRecord, GetOffset(), m_nSize));
		}
		virtual TFieldVal<BlobVal > GetAsBlob(const RecordData * pRecord) const
		{
			return TStorage::GetAsBlob(pRecord, GetOffset(), m_nSize);
		}
		
		virtual TFieldVal<BlobVal > GetAsSpatialBlob(const RecordData * pRecord) const
		{
			TFieldVal<BlobVal > ret;
#ifndef SRCLIB_REPLACEMENT
			unsigned int len = 0;
#endif

			TFieldVal<AStringVal > val = GetAsAString(m_storage.GetVal(pRecord, GetOffset(), m_nSize));

			ret.bIsNull = val.bIsNull || val.value.nLength==0;

			if (!ret.bIsNull)
			{
				try
				{
					if (m_pSpatialObj)
					{
						free(m_pSpatialObj);
						m_pSpatialObj = NULL;
					}
#ifndef SRCLIB_REPLACEMENT
					ConvertFromGeoJSON(val.value.pValue, &m_pSpatialObj, &len);
					if (len > 0)
					{
						ret.value.pValue = reinterpret_cast<unsigned char *>(m_pSpatialObj);
						ret.value.nLength = len;
						return ret;
					}
#else
					ret.bIsNull = true;
					return ret;
#endif
				}
				catch (...)
				{
					ret = TStorage::GetAsBlob(pRecord, GetOffset(), m_nSize);
				}
			}
			return ret;
		}

		virtual void SetFromInt32(Record *pRecord, int nVal) const
		{
			m_strBuffer.Assign(nVal);
			TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, m_strBuffer, m_strBuffer.Length());
		}
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const
		{
			m_strBuffer.Assign(nVal);
			TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, m_strBuffer, m_strBuffer.Length());
		}
		virtual void SetFromDouble(Record *pRecord, double dVal) const
		{
			m_strBuffer.Assign(dVal);
			TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, m_strBuffer, m_strBuffer.Length());
		}
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const
		{
			if (sizeof(TChar)==1)
				TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, reinterpret_cast<const TChar *>(pVal), nLen);
			else
			{
				ConvertString(m_strBuffer, pVal, unsigned(nLen));
				TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, m_strBuffer, nLen);
			}
		}
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const
		{
			if (sizeof(TChar)==2)
				TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, reinterpret_cast<const TChar *>(pVal), nLen);
			else
			{
				DoConvertString(m_strBuffer, pVal, unsigned(nLen));
				TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, m_strBuffer, nLen);
			}
		}
		virtual void SetFromBlob(Record *pRecord, const BlobVal & val) const
		{
			TStorage::SetVal(this, pRecord, GetOffset(), m_nSize, static_cast<const TChar *>(val.pValue), val.nLength);
		}
		virtual bool GetNull(const RecordData * pRecord) const
		{
			return TStorage::GetNull(pRecord, GetOffset(), m_nSize);
		}

		virtual void SetNull(Record *pRecord) const
		{
			TStorage::SetNull(pRecord, GetOffset(), m_nSize);
		}

		virtual void SetFromSpatialBlob(Record *pRecord, const BlobVal & val) const
		{
#ifdef SRCLIB_REPLACEMENT
			val;  // to get past compile warnings as errors
			SetNull(pRecord);
#else
			char *achTemp = NULL;
			if (val.pValue)
				achTemp = ConvertToGeoJSON(val.pValue, val.nLength);

			if (achTemp)
			{
				SetFromString(pRecord, achTemp, strlen(achTemp));
				free(achTemp);
			}
			else
				SetNull(pRecord);
#endif
		}


	};

	typedef T_Field_String<E_FT_String, char, Field_String_GetSet<char>, false > Field_String;
	typedef T_Field_String<E_FT_WString, wchar_t, Field_String_GetSet<wchar_t>, false > Field_WString;
	typedef T_Field_String<E_FT_V_String, char, Field_V_String_GetSet<char>, true > Field_V_String;
	typedef T_Field_String<E_FT_V_WString, wchar_t, Field_V_String_GetSet<wchar_t>, true > Field_V_WString;

	///////////////////////////////////////////////////////////////////////////////
	// class Date/Time
	class Field_DateTime_Base : public T_Field_String<E_FT_String, char, Field_String_GetSet<char>, false >
	{
		mutable AString m_strBuffer;
		template <class TChar> inline void Validate(Record *pRecord, const TChar * pVal) const
		{
			if (!Validate(GetAsAString(pRecord->GetRecord()).value))
			{
				SetNull(pRecord);
				if (GetGenericEngine())
					ReportFieldConversionError(L"\"" + ConvertToWString(pVal) + L"\" is not a valid " + GetNameFromFieldType(m_ft));
			}
		}
	protected:
		Field_DateTime_Base(WStringNoCase strFieldName, int nSize, E_FieldType ft)
			: T_Field_String<E_FT_String, char, Field_String_GetSet<char>, false >(strFieldName, nSize, -1, ft)
		{
		}

		virtual bool Validate(AStringVal val) const = 0;
	public:
		virtual void SetFromBool(Record *pRecord, bool bVal) const;
		virtual void SetFromInt32(Record *pRecord, int nVal) const;
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const;
		virtual void SetFromDouble(Record *pRecord, double dVal) const;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const;
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const;
	};
	class Field_Date : public Field_DateTime_Base
	{
	protected:
		virtual bool Validate(AStringVal val) const ;
	public:
		inline Field_Date(WStringNoCase strFieldName)
			: Field_DateTime_Base(strFieldName, 10, E_FT_Date)
		{
		}
		virtual SmartPointerRefObj<FieldBase> Copy() const;
	};
	class Field_Time : public Field_DateTime_Base
	{
	protected:
		virtual bool Validate(AStringVal val) const ;
	public:
		inline Field_Time(WStringNoCase strFieldName)
			: Field_DateTime_Base(strFieldName, 8, E_FT_Time)
		{
		}
		virtual SmartPointerRefObj<FieldBase> Copy() const;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const;
	};
	class Field_DateTime : public Field_DateTime_Base
	{
	protected:
		virtual bool Validate(AStringVal val) const ;
	public:
		inline Field_DateTime(WStringNoCase strFieldName)
			: Field_DateTime_Base(strFieldName, 19, E_FT_DateTime)
		{
		}
		virtual SmartPointerRefObj<FieldBase> Copy() const;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const;
	};
	///////////////////////////////////////////////////////////////////////////////
	// class Field_FixedDecimal
	class Field_FixedDecimal : public T_Field_String<E_FT_FixedDecimal, char, Field_String_GetSet<char>, false >
	{
	public:
		inline Field_FixedDecimal(WStringNoCase strFieldName, int nSize, int nScale)
			: T_Field_String<E_FT_FixedDecimal, char, Field_String_GetSet<char>, false >(strFieldName, nSize, nScale)
		{
		}
		virtual SmartPointerRefObj<FieldBase> Copy() const
		{
			return CopyHelper(new Field_FixedDecimal(GetFieldName(), m_nSize, m_nScale));
		}
		virtual TFieldVal<bool> GetAsBool(const RecordData * pRecord) const ;

		// GetAsInt32, GetAsInt64, GetAsDouble etc... are handled by the String (base) class

		virtual void SetFromBool(Record *pRecord, bool bVal) const;
		virtual void SetFromInt32(Record *pRecord, int nVal) const;
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const;
		virtual void SetFromDouble(Record *pRecord, double dVal) const;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const;
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const;
		virtual void SetFromBlob(Record *pRecord, const BlobVal & val) const;
	};

	///////////////////////////////////////////////////////////////////////////////
	// class Field_Blob
	class Field_Blob : public FieldBase
	{
	public:
		inline Field_Blob(WStringNoCase strFieldName, bool bIsSpatialObj)
			: FieldBase(strFieldName, bIsSpatialObj ? E_FT_SpatialObj : E_FT_Blob, 4, true, MaxFieldLength, 0)
		{}
		virtual SmartPointerRefObj<FieldBase> Copy() const
		{
			return CopyHelper(new Field_Blob(GetFieldName(), m_ft==E_FT_SpatialObj));
		}

		// unsupported accessors
		virtual TFieldVal<int> GetAsInt32(const RecordData * pRecord) const ;
		virtual TFieldVal<AStringVal > GetAsAString(const RecordData * pRecord) const;
		virtual TFieldVal<WStringVal > GetAsWString(const RecordData * pRecord) const;
		virtual void SetFromInt32(Record *pRecord, int nVal) const;
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const;
		virtual void SetFromDouble(Record *pRecord, double dVal) const;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const;
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const;

		// the only actually supported interfaces
		virtual TFieldVal<BlobVal > GetAsBlob(const RecordData * pRecord) const;
		virtual TFieldVal<BlobVal > GetAsSpatialBlob(const RecordData * pRecord) const ;
		virtual void SetFromBlob(Record *pRecord, const BlobVal & val) const;
		virtual void SetFromSpatialBlob(Record *pRecord, const BlobVal & val) const;
		virtual bool GetNull(const RecordData * pRecord) const;
		virtual void SetNull(Record *pRecord) const;
	};

}
#ifndef __GNUG__
#pragma warning(pop)
#endif

