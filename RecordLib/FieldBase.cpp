///////////////////////////////////////////////////////////////////////////////
//
// (C) 2005 SRC, LLC  -   All rights reserved
//
///////////////////////////////////////////////////////////////////////////////
//
// Module: FIELDBASE.CPP
//
///////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "FieldBase.h"
#include "FieldTypes.h"
#include "Record.h"
#ifndef SRCLIB_REPLACEMENT
#include "boost\date_time\posix_time\posix_time.hpp"
#include "..\..\inc\GeoJson.h"
#endif

#include "DateTimeValidate.h"


namespace SRC
{
	namespace {
		const wchar_t * const s_pE_FieldTypeNames[] = {
			L"Unknown",
			L"Bool",
			L"Byte",
			L"Int16",
			L"Int32",
			L"Int64",
			L"FixedDecimal",
			L"Float",
			L"Double",
			L"String",
			L"WString",
			L"V_String",
			L"V_WString",
			L"Date",
			L"Time",
			L"DateTime",
			L"Blob",
			L"SpatialObj",
			// GetFieldTypeFromName hardcodes this list - if change, must change there
		};
	}
	bool IsBool(E_FieldType eType)
	{
		return eType == E_FT_Bool;
	}
	bool IsNumeric(E_FieldType eType)
	{
		return 
			eType == E_FT_Byte ||
			eType == E_FT_Int16 ||
			eType == E_FT_Int32 ||
			eType == E_FT_Int64 ||
			eType == E_FT_FixedDecimal ||
			eType == E_FT_Float ||
			eType == E_FT_Double;
	}
	bool IsFloat(E_FieldType eType)
	{
		return 
			eType == E_FT_Float ||
			eType == E_FT_Double;
	}
	bool IsBoolOrInteger(E_FieldType eType)
	{
		return 
			eType == E_FT_Bool ||
			eType == E_FT_Byte ||
			eType == E_FT_Int16 ||
			eType == E_FT_Int32 ||
			eType == E_FT_Int64;
	}

	bool IsInteger(E_FieldType eType)
	{
		return 
			eType == E_FT_Byte ||
			eType == E_FT_Int16 ||
			eType == E_FT_Int32 ||
			eType == E_FT_Int64;
	}
	bool IsString(E_FieldType eType)
	{
		return eType == E_FT_String ||
			eType == E_FT_WString ||
			eType == E_FT_V_String ||
			eType == E_FT_V_WString;
	}
	bool IsStringOrDate(E_FieldType eType)
	{
		return eType == E_FT_String ||
			eType == E_FT_WString ||
			eType == E_FT_V_String ||
			eType == E_FT_V_WString ||
			eType == E_FT_Date ||
			eType == E_FT_Time ||
			eType == E_FT_DateTime;
	}
	bool IsDateOrTime(E_FieldType eType)
	{
		return eType == E_FT_Date ||
			eType == E_FT_Time ||
			eType == E_FT_DateTime;
	}
	bool IsTime(E_FieldType eType)
	{
		return eType == E_FT_Time;
	}

	bool IsDate(E_FieldType eType)
	{
		return eType == E_FT_Date ||
			eType == E_FT_DateTime;
	}

	bool IsBinary(E_FieldType eType)
	{
		return eType==E_FT_Blob || eType==E_FT_SpatialObj;
	}
	bool IsBlob(E_FieldType eType)
	{
		return eType==E_FT_Blob;
	}
	bool IsSpatialObj(E_FieldType eType)
	{
		return eType==E_FT_SpatialObj;
	}
	bool IsNotBinary(E_FieldType eType)
	{
		return eType!=E_FT_Blob && eType!=E_FT_SpatialObj;
	}
	bool IsNotBlob(E_FieldType eType)
	{
		return eType!=E_FT_Blob;
	}
	bool IsNotSpatial(E_FieldType eType)
	{
		return eType!=E_FT_SpatialObj;
	}
	bool IsWideString(E_FieldType eType)
	{
		return eType==E_FT_WString || eType==E_FT_V_WString;
	}

	unsigned GetMinimumStringSize(E_FieldType eType, int nSize)
	{
		switch (eType)
		{
		case E_FT_Bool:
			return 5;
		case E_FT_Byte:
			return 3;
		case E_FT_Int16:
			return 6;
		case E_FT_Int32:
			return 10;
		case E_FT_Int64:
			return 20;
		case E_FT_Float:
			return 13;
		case E_FT_Double:
			return 23;
		case E_FT_String:
		case E_FT_WString:
		case E_FT_V_String:
		case E_FT_V_WString:
		case E_FT_FixedDecimal:
			return nSize;
		case E_FT_Date:
			return 10;
		case E_FT_Time:
			return 8;
		case E_FT_DateTime:
			return 19;
		default:
		case E_FT_Blob:
		case E_FT_SpatialObj:
			assert(false);
			return nSize;
		}
	}



	const wchar_t * GetNameFromFieldType(E_FieldType ft)
	{
		return s_pE_FieldTypeNames[ft];
	}

#define ASSERTANDRETURNFT(Value, Type) { assert(_wcsicmp(Value, p) == 0); return Type; }
	E_FieldType GetFieldTypeFromName(const wchar_t *p)
	{
		switch (p[0])
		{
		case 'B':
		case 'b':
			if (p[1] == 'o' || p[1] == 'O')
				ASSERTANDRETURNFT(L"Bool", E_FT_Bool)
			else if (p[1] == 'l' || p[1] == 'L')
				ASSERTANDRETURNFT(L"Blob", E_FT_Blob)
			else
				ASSERTANDRETURNFT(L"Byte", E_FT_Byte)
		case 'I':
		case 'i':
			if ((p[1] == 'n' || p[1] == 'N') && (p[2] == 't' || p[2] == 'T'))
			{
				if (p[3] == '1')
					ASSERTANDRETURNFT(L"Int16", E_FT_Int16)
				else if (p[3] == '3')
					ASSERTANDRETURNFT(L"Int32", E_FT_Int32)
				else
					ASSERTANDRETURNFT(L"Int64", E_FT_Int64)
			}
			break;
		case 'F':
		case 'f':
			if (p[1] == 'i' || p[1] == 'I')
				ASSERTANDRETURNFT(L"FixedDecimal", E_FT_FixedDecimal)
			else
				ASSERTANDRETURNFT(L"Float", E_FT_Float)
		case 'D':
		case 'd':
			if (p[1] == 'o' || p[1] == 'O')
				ASSERTANDRETURNFT(L"Double", E_FT_Double)
			else if ((p[1] == 'a' || p[1] == 'A') && (p[2] == 't' || p[2] == 'T') && (p[3] == 'e' || p[3] == 'E'))
			{
				if (p[4] == 0)
					ASSERTANDRETURNFT(L"Date", E_FT_Date)
				else
					ASSERTANDRETURNFT(L"DateTime", E_FT_DateTime)
			}
			break;
		case 'S':
		case 's':
			if (p[1] == 't' || p[1] == 'T')
				ASSERTANDRETURNFT(L"String", E_FT_String)
			else
				ASSERTANDRETURNFT(L"SpatialObj", E_FT_SpatialObj)
		case 'W':
		case 'w':
			ASSERTANDRETURNFT(L"WString", E_FT_WString)
		case 'V':
		case 'v':
			if ((p[1] == '_'))
			{
				if (p[2] == 's' || p[2] == 'S')
					ASSERTANDRETURNFT(L"V_String", E_FT_V_String)
				else
					ASSERTANDRETURNFT(L"V_WString", E_FT_V_WString)
			}
			break;
		case 'T':
		case 't':
			ASSERTANDRETURNFT(L"Time", E_FT_Time)
		}
		assert(0 && "Unknown field type in GetFieldTypeFromName");
		return E_FT_Unknown;
	}

	//////////////////////////////////////////////////////////////
	// Default implementations of Accessors
	//////////////////////////////////////////////////////////////
	FieldBase::~FieldBase()
	{
	}

	void FieldBase::ReportFieldConversionError(const wchar_t * pMessage) const
	{
		// the calling function should not call if this is NULL
		assert(m_pGenericEngine);
		if (m_pGenericEngine)
		{
			m_pGenericEngine->OutputMessage(GenericEngineBase::MT_FieldConversionError, 
				this->m_strFieldName + L": " + pMessage);

			m_nFieldConversionErrorCount++;
			
			if (m_nFieldConversionErrorCount==m_pGenericEngine->GetFieldConversionErrorLimit())
			{
				m_pGenericEngine->OutputMessage(GenericEngineBase::MT_FieldConversionLimitReached, 
					this->m_strFieldName + L": Field Conversion Error Limit Reached");
				m_pGenericEngine = NULL;
			}
		}
	}

	/*virtual*/ unsigned FieldBase::GetMaxBytes() const
	{
		return m_nSize;
	}

	TFieldVal<bool> FieldBase::GetAsBool(const RecordData * pRecord)  const 
	{
		TFieldVal<bool> ret(false, false);
		switch (m_ft)
		{
		case E_FT_String:
		case E_FT_V_String:
			{
				TFieldVal<AStringVal > val = GetAsAString(pRecord);
				if (val.bIsNull)
					ret.value = Field_Bool::TestChar(val.value.pValue[0]);
				break;
			}
		case E_FT_WString:
		case E_FT_V_WString:
			{
				TFieldVal<WStringVal > val = GetAsWString(pRecord);
				if (val.bIsNull)
					ret.value = Field_Bool::TestChar(val.value.pValue[0]);
				break;
			}
		default:
			{
				TFieldVal<int> val = GetAsInt32(pRecord);
				ret.bIsNull = val.bIsNull;
				ret.value = val.value!=0;
				break;
			}
		}
		return ret;
	}
	TFieldVal<__int64> FieldBase::GetAsInt64(const RecordData * pRecord) const 
	{
		TFieldVal<int> val = GetAsInt32(pRecord);
		return TFieldVal<__int64>(val.bIsNull, static_cast<__int64>(val.value));
	}

	TFieldVal<double> FieldBase::GetAsDouble(const RecordData * pRecord) const 
	{
		TFieldVal<int> val = GetAsInt32(pRecord);
		return TFieldVal<double>(val.bIsNull, static_cast<double>(val.value));
	}

	TFieldVal<BlobVal > FieldBase::GetAsBlob(const RecordData * /*pRecord*/) const 
	{
		throw Error(String(L"SetFromBlob: Field type ") + GetNameFromFieldType(m_ft) + L" doesn't support Blobs.");
	}

	void FieldBase::SetFromBool(Record *pRecord, bool bVal) const
	{
		SetFromInt32(pRecord, bVal ? 1 : 0);
	}

	void FieldBase::SetFromBlob(Record * /*pRecord*/, const BlobVal & /*val*/) const
	{
		throw Error(String(L"SetFromBlob: Field type ") + GetNameFromFieldType(m_ft) + L" doesn't support Blobs.");
	}

	TFieldVal<BlobVal > FieldBase::GetAsSpatialBlob(const RecordData * /*pRecord */) const
	{
		throw Error(String(L"GetAsSpatialBlob: Field type ") + GetNameFromFieldType(m_ft) + L" doesn't support Spatial Objects.");
	}

	void FieldBase::SetFromSpatialBlob(Record * /*pRecord*/, const BlobVal & /*val*/) const
	{
		throw Error(String(L"SetFromSpatialBlob: Field type ") + GetNameFromFieldType(m_ft) + L" doesn't support Spatial Objects.");
	}


	///////////////////////////////////////////////////////////////////////////////
	// class Field_Bool
	//inline static bool TestChar(wchar_t c);
	TFieldVal<bool> Field_Bool::GetVal(const RecordData * pRecord) const 
	{
		TFieldVal<bool> ret(false, false);
		char c = *(ToCharP(pRecord)+GetOffset());
		ret.bIsNull = (c & 2)!=0; // Null
		if (!ret.bIsNull)
			ret.value = (c & 1)!=0;

		return ret;
	}

	TFieldVal<bool> Field_Bool::GetAsBool(const RecordData * pRecord) const 
	{
		return GetVal(pRecord);
	}
	TFieldVal<int> Field_Bool::GetAsInt32(const RecordData * pRecord) const 
	{
		TFieldVal<bool> val = GetVal(pRecord);
		return TFieldVal<int>(val.bIsNull, val.value);
	}
	
	TFieldVal<AStringVal > Field_Bool::GetAsAString(const RecordData * pRecord) const 
	{
		TFieldVal<bool> val = GetVal(pRecord);
		TFieldVal<AStringVal > ret(true, AStringVal(0u, ""));
		if (!val.bIsNull)
		{
			m_astrTemp = val.value ? "True" : "False";
			ret.value = AStringVal(m_astrTemp.Length(), m_astrTemp.c_str());
			ret.bIsNull = false;
		}
		return ret;
	}

	TFieldVal<WStringVal > Field_Bool::GetAsWString(const RecordData * pRecord) const 
	{
		TFieldVal<bool> val = GetVal(pRecord);
		TFieldVal<WStringVal > ret(true, WStringVal(0u, L""));
		if (!val.bIsNull)
		{
			m_wstrTemp = val.value ? L"True" : L"False";
			ret.value = WStringVal(m_wstrTemp.Length(), m_wstrTemp.c_str());
			ret.bIsNull = false;
		}
		return ret;
	}

	void Field_Bool::SetVal(Record *pRecord, bool bVal) const
	{
		// the by default resets the Null flag.
		*(ToCharP(pRecord->GetRecord())+GetOffset()) = bVal ? 1 : 0;
	}

	void Field_Bool::SetFromBool(Record *pRecord, bool bVal) const
	{
		SetVal(pRecord, bVal);
	}
	void Field_Bool::SetFromInt32(Record *pRecord, int nVal) const
	{
		SetVal(pRecord, nVal!=0);
	}
	void Field_Bool::SetFromInt64(Record *pRecord, __int64 nVal) const
	{
		SetVal(pRecord, nVal!=0);
	}
	void Field_Bool::SetFromDouble(Record *pRecord, double dVal) const
	{
		SetVal(pRecord, dVal!=0);
	}
	void Field_Bool::SetFromString(Record *pRecord, const char * pVal, size_t /*nLen*/) const
	{
		SetVal(pRecord, TestChar(*pVal));
	}
	void Field_Bool::SetFromString(Record *pRecord, const wchar_t * pVal, size_t /*nLen*/) const
	{
		SetVal(pRecord, TestChar(*pVal));
	}

	bool Field_Bool::GetNull(const RecordData * pRecord) const
	{
		return GetVal(pRecord).bIsNull;
	}

	void Field_Bool::SetNull(Record *pRecord) const
	{
		*(ToCharP(pRecord->GetRecord())+GetOffset()) = 2;
	}

	// explicitly instantiate all the template types
#ifndef __GNUG__
	template Field_Byte;
	template Field_Int16;
	template Field_Int32;
	template Field_Int64;
	template Field_String; 
	template Field_WString;
	template Field_V_String; 
	template Field_V_WString;
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class Field_Blob
	/*virtual*/ TFieldVal<int> Field_Blob::GetAsInt32(const RecordData * /*pRecord*/) const
	{
		throw Error("Internal Error in Field_Blob::GetAsInt32: Not supported.");
	}
	/*virtual*/ TFieldVal<AStringVal > Field_Blob::GetAsAString(const RecordData * pRecord) const
	{
		TFieldVal<BlobVal > blob = GetAsBlob(pRecord);
		TFieldVal<AStringVal > ret(true, AStringVal(0u, ""));
		if (!blob.bIsNull)
		{
#ifdef SRCLIB_REPLACEMENT
			m_astrTemp = "SpatialObject";
#else
			if (m_ft==E_FT_SpatialObj)
			{
				char *achTemp = ConvertToGeoJSON(blob.value.pValue, blob.value.nLength);
				if (achTemp)
				{
					m_astrTemp = achTemp;
					free(achTemp);
				}
				else
					m_astrTemp = "[Null]";
			}
			else
			{
				m_astrTemp.Assign(blob.value.nLength);
				m_astrTemp += " Bytes";
			}
#endif
			ret.bIsNull = false;
		}
		else
			m_astrTemp = "[Null]";
		ret.value = AStringVal(m_astrTemp.Length(), m_astrTemp.c_str());
		return ret;
	}	
	/*virtual*/ TFieldVal<WStringVal > Field_Blob::GetAsWString(const RecordData * pRecord) const
	{
		TFieldVal<BlobVal > blob = GetAsBlob(pRecord);
		TFieldVal<WStringVal > ret(true, WStringVal(0u, L""));
		if (!blob.bIsNull)
		{
#ifdef SRCLIB_REPLACEMENT
			m_wstrTemp = L"SpatialObject";
#else
			if (m_ft==E_FT_SpatialObj)
			{
				char *achTemp = ConvertToGeoJSON(blob.value.pValue, blob.value.nLength);
				if (achTemp)
				{
					m_wstrTemp = ConvertToWString(achTemp);
					free(achTemp);
				}		
				else
					m_wstrTemp = L"[Null]";
			}
			else
			{
				m_wstrTemp.Assign(blob.value.nLength);
				m_wstrTemp += L" Bytes";
			}
#endif
			ret.bIsNull = false;
		}
		else
			m_wstrTemp = L"[Null]";
		ret.value = WStringVal(m_wstrTemp.Length(), m_wstrTemp.c_str());
		return ret;
	}	

	/*virtual*/ void Field_Blob::SetFromInt32(Record *  /*pRecord*/, int /*nVal*/) const
	{
		throw Error("Internal Error in Field_Blob::SetFromInt32: Not supported.");
	}
	/*virtual*/ void Field_Blob::SetFromInt64(Record * /*pRecord*/, __int64 /*nVal*/) const
	{
		throw Error("Internal Error in Field_Blob::SetFromInt64: Not supported.");
	}
	/*virtual*/ void Field_Blob::SetFromDouble(Record * /*pRecord*/, double /*dVal*/) const
	{
		throw Error("Internal Error in Field_Blob::SetFromDouble: Not supported.");
	}
	/*virtual*/ void Field_Blob::SetFromString(Record * /*pRecord*/, const char * /*pVal*/, size_t /*nLen*/) const
	{
		throw Error("Internal Error in Field_Blob::SetFromString: Not supported.");
	}
	/*virtual*/ void Field_Blob::SetFromString(Record * /*pRecord*/, const wchar_t * /*pVal*/, size_t /*nLen*/) const
	{
		throw Error("Internal Error in Field_Blob::SetFromString: Not supported.");
	}

	/*virtual*/ TFieldVal<BlobVal > Field_Blob::GetAsBlob(const RecordData * pRecord) const
	{
		TFieldVal<BlobVal > ret(false, RecordInfo::GetVarDataValue(pRecord, GetOffset()));
		ret.bIsNull = ret.value.pValue==NULL;
		return ret;
	}

	/*virtual*/ TFieldVal<BlobVal > Field_Blob::GetAsSpatialBlob(const RecordData * pRecord) const
	{
		//validate that it is a correct spatial object
		TFieldVal<BlobVal > ret(false, RecordInfo::GetVarDataValue(pRecord, GetOffset()));
		ret.bIsNull = ret.value.pValue==NULL;
		if (!ret.bIsNull)
		{
			BlobDataRead file(ret.value.pValue, ret.value.nLength);
			bool bValid = SHPBlob::ValidateShpBlob(file);
			if (!bValid)
				throw Error("Internal Error in Field_Blob::GetAsSpatialBlob: Invalid SpatialBlob.");
		}
		return ret;
	}

	/*virtual*/ void Field_Blob::SetFromBlob(Record *pRecord, const BlobVal & val) const
	{
		RecordInfo::SetVarDataValue(pRecord, GetOffset(), val.nLength, val.pValue);
	}

	/*virtual*/ void Field_Blob::SetFromSpatialBlob(Record *pRecord, const BlobVal & val) const
	{
		RecordInfo::SetVarDataValue(pRecord, GetOffset(), val.nLength, val.pValue);
	}

	bool Field_Blob::GetNull(const RecordData * pRecord) const
	{
		return GetAsBlob(pRecord).bIsNull;
	}

	/*virtual*/ void Field_Blob::SetNull(Record *pRecord) const
	{
		RecordInfo::SetVarDataValue(pRecord, GetOffset(), 0, NULL);
	}
	///////////////////////////////////////////////////////////////////////////////
	// class Field_FixedDecimal
	/*virtual*/ TFieldVal<bool> Field_FixedDecimal::GetAsBool(const RecordData * pRecord) const
	{
		// this will NOT be null terminated;
		TFieldVal<BlobVal > ret = GetAsBlob(pRecord);
		if (ret.bIsNull)
			return TFieldVal<bool>(true, false);

		// FixedDecimal derives from a narrow string
		const char * p = static_cast<const char *>(ret.value.pValue);
		for (unsigned x = 0; x < ret.value.nLength; ++x)
		{
			char c = p[x];
			if (c>='1' && c<='9')
				return TFieldVal<bool>(false, true);
		}
		return TFieldVal<bool>(false, false);
	}

	/*virtual*/ void Field_FixedDecimal::SetFromBool(Record *pRecord, bool bVal) const
	{
		SetFromDouble(pRecord, bVal ? 1.0 : 0.0);
	}

	/*virtual*/ void Field_FixedDecimal::SetFromDouble(Record *pRecord, double dVal) const
	{
		m_astrTemp.Assign(dVal, m_nScale);
		if (m_astrTemp.Length()>m_nSize)
		{
			SetNull(pRecord);
			if (m_pGenericEngine)
			{
				WString strError = L"\"";
				strError += ConvertToWString(m_astrTemp);
				strError += L"\" does not fit in Fixed Decimal " + WString().Assign(int(m_nSize)) + L"."  + WString().Assign(int(m_nScale));
				ReportFieldConversionError(strError);
			}
		}
		else
			Field_String_GetSet<char>::SetVal(this, pRecord, GetOffset(), m_nSize, m_astrTemp, m_astrTemp.Length());
	}
	/*virtual*/ void Field_FixedDecimal::SetFromInt32(Record *pRecord, int nVal) const
	{
		SetFromDouble(pRecord, double(nVal));
	}
	/*virtual*/ void Field_FixedDecimal::SetFromInt64(Record *pRecord, __int64 nVal) const
	{
		m_astrTemp.Assign(nVal);
		if (m_astrTemp.Length()>m_nSize)
		{
			SetNull(pRecord);
			if (m_pGenericEngine)
			{
				WString strError = L"\"";
				strError += ConvertToWString(m_astrTemp);
				strError += L"\" does not fit in Fixed Decimal " + WString().Assign(int(m_nSize)) + L"."  + WString().Assign(int(m_nScale));
				ReportFieldConversionError(strError);
			}
		}
		else
			SetFromString(pRecord, m_astrTemp, m_astrTemp.Length());
	}
	/*virtual*/ void Field_FixedDecimal::SetFromString(Record *pRecord, const char * pOrigVal, size_t nLenOrig) const
	{
		if (nLenOrig==0)
		{
			SetNull(pRecord);
			return;
		}
		// if it goes through as a wide string 1st, this can happen.
		// I really mean to do pointer comparison here...
		if (pOrigVal!=m_astrTemp.c_str())
		{
			m_astrTemp.Truncate(0);
			m_astrTemp.Append(pOrigVal, unsigned(nLenOrig)) ;
		}
		if (m_astrTemp.c_str()[0]=='.')
			m_astrTemp = "0" + m_astrTemp;
		else if (m_astrTemp.c_str()[0]=='-' && m_astrTemp.c_str()[1]=='.')
			m_astrTemp = AString("-0") + (m_astrTemp.c_str()+1);
		const char *pVal = m_astrTemp;
		size_t nLen = m_astrTemp.Length();

		// validate the incoming string 1st.
		const char *pEnd = pVal + nLen;
		const char *p = pVal;
		if (*p=='+' || *p=='-')
			p++;
		bool bInvalid = !isdigit(*p);
		int nNumAfterDecimal = 0;
		if (!bInvalid)
		{
			while (p<pEnd && isdigit(*p))
				++p;

			if (*p=='.')
			{
				++p;
				bInvalid = p==pEnd;
				while (p<pEnd && isdigit(*p))
				{
					++nNumAfterDecimal;
					++p;
				}
			}
		}

		if (p!=pEnd || bInvalid)
		{
			SetNull(pRecord);
			if (m_pGenericEngine)
			{
				WString strError = L"\"";
				strError += ConvertToWString(pVal);
				strError += L"\" is not a valid FixedDecimal.  FixedDecimal values must be of the form: -nnn.nn";
				ReportFieldConversionError(strError);
			}
			return;
		}

		if (nNumAfterDecimal>this->m_nScale)
		{
			int nNewLen = m_astrTemp.Length()-(nNumAfterDecimal-this->m_nScale);
			char droppedDigit = m_astrTemp[unsigned(nNewLen)];
			
			if (this->m_nScale==0)
			{
				--nNewLen; // get rid of the .
			}
		
			m_astrTemp.Truncate(nNewLen);

			if (droppedDigit >='5' && droppedDigit <='9')
			{
				char * pRoundVal = m_astrTemp.Lock();
				int digitToRoundUp = nNewLen-1;
				for (; ;)
				{
					if (digitToRoundUp < 0)
					{
						m_astrTemp.Unlock();
						m_astrTemp = "1" + m_astrTemp;
						break;
					}
					
					switch (pRoundVal[digitToRoundUp] )
					{
					case '+':
						pRoundVal[digitToRoundUp] = '1';
						m_astrTemp.Unlock();
						break;
					case '-':
						m_astrTemp.Unlock();
						assert(digitToRoundUp ==0);
						m_astrTemp = AString("-1") + (m_astrTemp.c_str() +digitToRoundUp+1) ;
						break;
					case '.':
						digitToRoundUp--;
						continue;
					case '9':
						pRoundVal[digitToRoundUp] = '0';
						digitToRoundUp--;
						continue;
					case '0':
						pRoundVal[digitToRoundUp] = '1';
						m_astrTemp.Unlock();
						break;
					default:
						pRoundVal[digitToRoundUp]++;
						m_astrTemp.Unlock();
						break;
					}
					break;
				}
			}

			if (m_pGenericEngine)
			{
				AString strTemp = pOrigVal;
				strTemp.Truncate(nLenOrig > 64 ? 64 : unsigned(nLenOrig));
				if (nLenOrig > 64) 
					strTemp += "...";

				WString strError = L"\"";
				strError += ConvertToWString(strTemp);
				strError += L"\" has too many digits after the decimal and was truncated.";
				ReportFieldConversionError(strError);
			}
		}
		if (nNumAfterDecimal==0 && this->m_nScale!=0)
			m_astrTemp += '.';

		while (nNumAfterDecimal<this->m_nScale)
		{
			m_astrTemp += '0';
			++nNumAfterDecimal;
		}

		if (m_astrTemp.Length()>this->m_nSize)
		{
			SetNull(pRecord);
			if (m_pGenericEngine)
			{
				WString strError = L"\"";
				strError += ConvertToWString(m_astrTemp);
				strError += L"\" was too long to fit in this FixedDecimal";
				ReportFieldConversionError(strError);
			}
			return;		
		}
		Field_String_GetSet<char>::SetVal(this, pRecord, GetOffset(), m_nSize, m_astrTemp, m_astrTemp.Length());

	}
	/*virtual*/ void Field_FixedDecimal::SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const
	{
		ConvertString(m_astrTemp, pVal, unsigned(nLen));
		SetFromString(pRecord, m_astrTemp.c_str(), m_astrTemp.Length());
	}
	/*virtual*/ void Field_FixedDecimal::SetFromBlob(Record * /*pRecord*/, const BlobVal & /*val*/) const
	{
		throw Error("Internal Error in Field_FixedDecimal::SetFromBlob: Not supported.");
	}

	///////////////////////////////////////////////////////////////////////////////
	// class Field_Date 
	/*virtual*/ void Field_DateTime_Base::SetFromBool(Record * /*pRecord*/, bool /*bVal*/) const
	{
		throw Error("Date/Time fields do not support Conversion from Bool");
	}
	/*virtual*/ void Field_DateTime_Base::SetFromInt32(Record * /*pRecord*/, int /*nVal*/) const
	{
		throw Error("Date/Time fields do not support Conversion from Int32");
	}
	/*virtual*/ void Field_DateTime_Base::SetFromInt64(Record * /*pRecord*/, __int64 /*nVal*/) const
	{
		throw Error("Date/Time fields do not support Conversion from Int64");
	}

	/*virtual*/ void Field_DateTime_Base::SetFromDouble(Record * pRecord, double dVal) const
	{
#ifndef SRCLIB_REPLACEMENT
		auto WriteNumber4 = [](char *pDestLSB, int nValue)
		{
			*pDestLSB = nValue % 10 + '0';
			--pDestLSB;
			nValue /= 10;
	
			*pDestLSB = nValue % 10 + '0';
			--pDestLSB;
			nValue /= 10;

			*pDestLSB = nValue % 10 + '0';
			--pDestLSB;
			nValue /= 10;

			*pDestLSB = nValue % 10 + '0';
		};

		auto WriteNumber2 = [](char *pDestLSB, int nValue)
		{
			*pDestLSB = nValue % 10 + '0';
			--pDestLSB;
			nValue /= 10;

			*pDestLSB = nValue % 10 + '0';
		};

		if (dVal < 0 || dVal >= 2958466) // double value must be between 12/30/1899 and 12/31/9999
		{
			SetNull(pRecord);
			if (m_pGenericEngine)
			{
				WString strError = L"\"";
				strError += String(dVal);
				strError += L"\" is an invalid datetime - ";
				if (dVal < 0)
					strError += L"Earliest date supported is Dec 30, 1899";
				else
					strError += L"Latest date supported is Dec 31, 9999";
				ReportFieldConversionError(strError);
			}
			return;	
		}
		
		char buffer[20];
		const boost::gregorian::date d1(1899,12,30);							// create date that is == 0
		boost::gregorian::date_duration dd(static_cast<int>(dVal));	// strip the time portion from double
		boost::gregorian::greg_year_month_day ymd = (d1 + dd).year_month_day();	// add our date to zero, to get YMD

		// Offsetting by a 1/2 second to avoid rounding errors below
		dVal += 0.5 / (24 * 60 * 60);

		// logic to calculate the time
		double days, hours, mins, secs; 
		double hoursmins = modf(dVal, &days) * 24.0;
		double minssecs = modf(hoursmins, &hours) * 60.0;
		double secsrem = modf(minssecs, &mins) * 60.0;
		modf(secsrem, &secs);

		WriteNumber4(buffer + 3, ymd.year);
		buffer[4] = '-';
		WriteNumber2(buffer + 6, ymd.month);
		buffer[7] = '-';
		WriteNumber2(buffer + 9, ymd.day);
		buffer[10] = ' ';

		WriteNumber2(buffer + 12, static_cast<int>(hours + 0.5));
		buffer[13] = ':';

		WriteNumber2(buffer + 15, static_cast<int>(mins + 0.5));
		buffer[16] = ':';

		WriteNumber2(buffer + 18, static_cast<int>(secs + 0.5));
		buffer[19] = 0;
		SetFromString(pRecord, buffer, 19);
#else
		pRecord;
		dVal;
		throw Error("Date/Time fields do not support Conversion from Double");
#endif
	}
	/*virtual*/ void Field_DateTime_Base::SetFromString(Record *pRecord, const char * pVal, size_t nLen) const
	{
		nLen = std::min(unsigned(nLen), m_nSize);
		T_Field_String<E_FT_String, char, Field_String_GetSet<char>, false >::SetFromString(pRecord, pVal, nLen);
		Validate(pRecord, pVal);
	}
	/*virtual*/ void Field_DateTime_Base::SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const
	{
		DoConvertString(m_strBuffer, pVal, int(nLen));
		SetFromString(pRecord, m_strBuffer.c_str(), m_strBuffer.Length());
	}

	bool ValidateDate(const char *pVal, int nLen)
	{
		return TDateTimeValidate<char>::ValidateDate(pVal, nLen);
	}
	bool ValidateTime(const char *pVal, int nLen)
	{
		return TDateTimeValidate<char>::ValidateTime(pVal, nLen);
	}
	bool ValidateDateTime(const char *pVal, int nLen)
	{
		return TDateTimeValidate<char>::ValidateDateTime(pVal, nLen);
	}

	///////////////////////////////////////////////////////////////////////////////
	// class Field_Date 
	/*virtual*/ bool Field_Date::Validate(AStringVal val) const
	{
		return ValidateDate(val.pValue, val.nLength);
	}
	/*virtual*/ SmartPointerRefObj<FieldBase> Field_Date::Copy() const
	{
		return CopyHelper(new Field_Date(GetFieldName()));
	}
	///////////////////////////////////////////////////////////////////////////////
	// class Field_Time 
	/*virtual*/ bool Field_Time::Validate(AStringVal val) const
	{
		return ValidateTime(val.pValue, val.nLength);
	}
	/*virtual*/ SmartPointerRefObj<FieldBase> Field_Time::Copy() const
	{
		return CopyHelper(new Field_Time(GetFieldName()));
	}
	/*virtual*/ void Field_Time::SetFromString(Record *pRecord, const char * pVal, size_t nLen) const
	{
		if (nLen==19 && ValidateDate(pVal, 10))
		{
			pVal += 11;
			nLen -= 11;
		}
		Field_DateTime_Base::SetFromString(pRecord, pVal, nLen);
	}

	///////////////////////////////////////////////////////////////////////////////
	// class Field_DateTime 
	/*virtual*/ bool Field_DateTime::Validate(AStringVal val) const
	{
		return ValidateDateTime(val.pValue, val.nLength);
	}
	/*virtual*/ SmartPointerRefObj<FieldBase> Field_DateTime::Copy() const
	{
		return CopyHelper(new Field_DateTime(GetFieldName()));
	}
	/*virtual*/ void Field_DateTime::SetFromString(Record *pRecord, const char * pVal, size_t nLen) const
	{
		if (nLen==10)
		{
			AString temp = pVal;
			temp += " 00:00:00";
			Field_DateTime_Base::SetFromString(pRecord, temp.c_str(), temp.Length());
		}
		else
			Field_DateTime_Base::SetFromString(pRecord, pVal, nLen);
	}

}

