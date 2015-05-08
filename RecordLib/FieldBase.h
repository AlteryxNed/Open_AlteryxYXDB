///////////////////////////////////////////////////////////////////////////////
//
// (C) 2005 SRC, LLC  -   All rights reserved
//
///////////////////////////////////////////////////////////////////////////////
//
// Module: FIELDBASE.H
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#include "RecordObj.h"
#include <limits>

namespace SRC
{
	enum E_FieldType
	{
		E_FT_Unknown,  // not really a type
		E_FT_Bool,
		E_FT_Byte, //unsigned byte
		E_FT_Int16,
		E_FT_Int32,
		E_FT_Int64,
		E_FT_FixedDecimal,  
		E_FT_Float,
		E_FT_Double,
		E_FT_String,
		E_FT_WString,
		E_FT_V_String,
		E_FT_V_WString,
		E_FT_Date, // A 10 char String in "yyyy-mm-dd" format
		E_FT_Time, // A 8 char String in "hh:mm:ss" format
		E_FT_DateTime, // A 19 char String in "yyyy-mm-dd hh:mm:ss" format
		E_FT_Blob,
		E_FT_SpatialObj,
	} ;

	typedef bool (*IsWhat)(E_FieldType eType);
	bool IsBool(E_FieldType eType);
	bool IsBoolOrInteger(E_FieldType eType);
	bool IsInteger(E_FieldType eType);
	bool IsFloat(E_FieldType eType);
	bool IsNumeric(E_FieldType eType);
	bool IsString(E_FieldType eType);
	bool IsStringOrDate(E_FieldType eType);
	bool IsDateOrTime(E_FieldType eType);
	bool IsTime(E_FieldType eType);
	bool IsDate(E_FieldType eType);
	bool IsBinary(E_FieldType eType);
	bool IsBlob(E_FieldType eType);
	bool IsSpatialObj(E_FieldType eType);
	bool IsNotBinary(E_FieldType eType);
	bool IsNotBlob(E_FieldType eType);
	bool IsNotSpatial(E_FieldType eType);
	bool IsWideString(E_FieldType eType);

	unsigned GetMinimumStringSize(E_FieldType eType, int nSize);

	bool ValidateDate(const char *pVal, int nLen);
	bool ValidateTime(const char *pVal, int nLen);
	bool ValidateDateTime(const char *pVal, int nLen);


	const wchar_t * GetNameFromFieldType(E_FieldType ft);
	E_FieldType GetFieldTypeFromName(const wchar_t *p);

	// Wide strings are limited to half of this since it is in bytes, not characters
	const unsigned MaxFixedLengthStringSize = 16384;

	// MAxFieldLength is also the Max Record length.  In reality the max field length would be a at least 12 bytes less, because you have:
	//		the field offset value
	//		the total vardata length
	//		the field vardata length
	const unsigned MaxFieldLength64 = 0x7fffffffu; // 2GB
	const unsigned MaxFieldLength32 = 0x0fffffffu; // 256MB

#ifdef _WIN64
	const unsigned MaxFieldLength = MaxFieldLength64;
#else
	const unsigned MaxFieldLength = MaxFieldLength32;
#endif
	
	class Record;
	///////////////////////////////////////////////////////////////////////////////
	//	class GenericEngineBase
	class GenericEngineBase
	{
	protected:
		mutable unsigned m_nFieldConversionErrorLimit;
	public:
		inline GenericEngineBase(unsigned nFieldConversionErrorLimit)
			: m_nFieldConversionErrorLimit(nFieldConversionErrorLimit)
		{
			if (m_nFieldConversionErrorLimit==0)
				m_nFieldConversionErrorLimit = std::numeric_limits<unsigned>::max();
		}
		virtual ~GenericEngineBase()
		{
		}
		inline unsigned GetFieldConversionErrorLimit() const { return m_nFieldConversionErrorLimit; }
		enum MessageType
		{
			// these ID's are set to these values just to make it easy on Alteryx and no harder on other apps.
			MT_Info = 1,
			MT_Warning = 2,
			MT_FieldConversionError = 5,
			MT_FieldConversionLimitReached = 6,
			MT_Error = 3,
			MT_SafeModeError = 21,
		};
		virtual long OutputMessage(MessageType mt, const wchar_t *pMessage) const = 0;

		typedef void ( __stdcall * ThreadProc )(void *pData);
		virtual void QueueThread(ThreadProc pProc, void *pData) const = 0;

		virtual const wchar_t * GetInitVar2(int /*nToolId*/, const wchar_t * /*pVar*/)
		{
			return L"";
		}

		// returns true to cancel processing
		// this can be called if there will be long periods of processing with no progress to the user
		virtual bool Ping() const = 0;
	};
	class NullEngine
	{
	public:
		typedef void ( __stdcall * ThreadProc )(void *pData);
		virtual void QueueThread(ThreadProc /*pProc*/, void * /*pData*/) const
		{
		}
		virtual bool Ping() const
		{
			return true;
		}
	};


	template <class ValType> struct TFieldVal
	{
		bool bIsNull;
		ValType value;

		inline TFieldVal()
			: bIsNull(false)
		{
		}

		inline TFieldVal(bool _bIsNull, ValType _value)
			: bIsNull(_bIsNull)
			, value(_value)
		{
		}
	};

	template <class ValType> struct TBlobVal
	{
		unsigned nLength;
		const ValType * pValue;
		inline TBlobVal()
			: nLength(0)
			, pValue(NULL)
		{
		}
		inline TBlobVal(unsigned _nLength, const ValType * _pValue)
			: nLength(_nLength)
			, pValue(_pValue)
		{
		}
	};
	template <> inline TBlobVal<char>::TBlobVal()
			: nLength(0)
			, pValue("")
	{
	}
	template <> inline TBlobVal<wchar_t>::TBlobVal()
			: nLength(0)
			, pValue(L"")
	{
	}
	typedef TBlobVal<wchar_t> WStringVal;
	typedef TBlobVal<char> AStringVal;
	typedef TBlobVal<void> BlobVal;


	////////////////////////////////////////////////////////////////////////////////////////
	// class FieldBase
	//
	// This class is not thread safe - even when calling const functions.
	// It uses internal buffers for managing the translations and 2 threads can't be using it at
	// the same time
	////////////////////////////////////////////////////////////////////////////////////////
	class FieldBase : public SmartPointerRefObj_Base
	{
		friend class RecordInfo;
		friend class RecordCopier;
		unsigned m_nOffset;
		// the field name is about the only thing that can be safely changed after th field has been created
		mutable WStringNoCase m_strFieldName;
		mutable WString m_strSource;
		mutable WString m_strDescription;
		inline void SetFieldName(WStringNoCase str) const { m_strFieldName = str; }

	protected:
		inline FieldBase(const WStringNoCase strFieldName, E_FieldType ft, const int nRawSize, bool bIsVarLength, int nSize, int nScale)
			: m_nOffset(0)
			, m_strFieldName(strFieldName)
			, m_pGenericEngine(NULL)
			, m_nFieldConversionErrorCount(0)
			, m_ft(ft)
			, m_nRawSize(nRawSize)
			, m_bIsVarLength(bIsVarLength)
			, m_nSize(nSize)
			, m_nScale(nScale)
		{
		}

		FieldBase * CopyHelper(FieldBase *p) const
		{
			p->m_nOffset = m_nOffset;
			p->SetSource(m_strSource);
			p->SetDescription(m_strDescription);
			return p;
		}

		FieldBase(const FieldBase&);
		FieldBase & operator =(const FieldBase&);

		mutable AString m_astrTemp;
		mutable WString m_wstrTemp;

		mutable const GenericEngineBase * m_pGenericEngine;
		mutable unsigned m_nFieldConversionErrorCount;


	public:
		virtual ~FieldBase();

		virtual SmartPointerRefObj<FieldBase> Copy() const = 0;

		inline WStringNoCase GetFieldName() const { return m_strFieldName; }
		inline WString GetSource() const { return m_strSource; }

		// the convention of the Source is the it has a scope identifier and a value
		// the value is interpretable only in that scope - the scope string cannot contain a :
		inline void SetSource(WString strSource) const { m_strSource = strSource; }
		inline void SetSource(WString strScope, WString strValue) const { assert(wcschr(strScope.c_str(), L':')==NULL); m_strSource = strScope + L":" + strValue; }
		inline WString GetDescription() const { return m_strDescription; }
		inline void SetDescription(WString strDescription) const { m_strDescription = strDescription; }

		
		const E_FieldType m_ft;
		const unsigned m_nRawSize;
		const bool m_bIsVarLength;

		const unsigned m_nSize;
		const int m_nScale;
		inline int GetOffset() const {return m_nOffset; }
		virtual unsigned GetMaxBytes() const ;

		inline bool operator ==(const FieldBase &o) const
		{
			return m_ft==o.m_ft && 
				m_strFieldName==o.m_strFieldName && 
				m_nRawSize==o.m_nRawSize && 
				m_bIsVarLength==o.m_bIsVarLength && 
				m_nSize==o.m_nSize && 
				m_nScale==o.m_nScale;
		}
		inline bool operator !=(const FieldBase &o) const
		{
			return !(*this==o);
		}

		// returns true if everything about the field is the same
		// other than the name can be different
		inline bool EqualType(const FieldBase &o) const
		{
			return m_ft==o.m_ft && 
				m_nRawSize==o.m_nRawSize && 
				m_bIsVarLength==o.m_bIsVarLength && 
				m_nSize==o.m_nSize && 
				m_nScale==o.m_nScale;
		}

		virtual TFieldVal<bool> GetAsBool(const RecordData * pRecord) const ;
		virtual TFieldVal<int> GetAsInt32(const RecordData * pRecord) const  = 0;
		virtual TFieldVal<__int64> GetAsInt64(const RecordData * pRecord) const ;
		virtual TFieldVal<double> GetAsDouble(const RecordData * pRecord) const ;
		virtual TFieldVal<AStringVal > GetAsAString(const RecordData * pRecord) const  = 0;
		virtual TFieldVal<WStringVal > GetAsWString(const RecordData * pRecord) const  = 0;
		virtual TFieldVal<BlobVal > GetAsBlob(const RecordData * pRecord) const ;
		virtual TFieldVal<BlobVal > GetAsSpatialBlob(const RecordData * pRecord) const ;
		
		virtual void SetFromBool(Record *pRecord, bool bVal) const;
		virtual void SetFromInt32(Record *pRecord, int nVal) const = 0;
		virtual void SetFromInt64(Record *pRecord, __int64 nVal) const = 0;
		virtual void SetFromDouble(Record *pRecord, double dVal) const = 0;
		virtual void SetFromString(Record *pRecord, const char * pVal, size_t nLen) const = 0;
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal, size_t nLen) const = 0;
		virtual void SetFromBlob(Record *pRecord, const BlobVal & val) const;
		virtual void SetFromSpatialBlob(Record *pRecord, const BlobVal & val) const ;

		// helpers for dealing with copying the NULL
		inline  void SetFromBool(Record *pRecord, const TFieldVal<bool> val) const
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromBool(pRecord, val.value);
		}
		inline void SetFromInt32(Record *pRecord, const TFieldVal<int> & val) const 
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromInt32(pRecord, val.value);
		}
		inline void SetFromInt64(Record *pRecord, const TFieldVal<__int64> & val) const 
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromInt64(pRecord, val.value);
		}
		inline void SetFromDouble(Record *pRecord, const TFieldVal<double> & val) const 
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromDouble(pRecord, val.value);
		}
		inline void SetFromString(Record *pRecord, const TFieldVal<AStringVal > & val) const 
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromString(pRecord, val.value.pValue, val.value.nLength);
		}
		inline void SetFromString(Record *pRecord, const TFieldVal<WStringVal > & val) const 
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromString(pRecord, val.value.pValue, val.value.nLength);
		}
		inline void SetFromString(Record *pRecord, const AString &strVal) const
		{
			SetFromString(pRecord, strVal.c_str(), strVal.Length());
		}
		inline void SetFromString(Record *pRecord, const WString &strVal) const
		{
			SetFromString(pRecord, strVal.c_str(), strVal.Length());
		}
		inline void SetFromString(Record *pRecord, const char * pVal) const
		{
			SetFromString(pRecord, pVal, strlen(pVal));
		}
		virtual void SetFromString(Record *pRecord, const wchar_t * pVal) const
		{
			SetFromString(pRecord, pVal, wcslen(pVal));
		}

		inline void SetFromBlob(Record *pRecord, const TFieldVal<BlobVal > & val) const
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromBlob(pRecord, val.value);
		}
		inline void SetFromSpatialBlob(Record *pRecord, const TFieldVal<BlobVal > & val) const
		{
			if (val.bIsNull)
				SetNull(pRecord);
			else
				SetFromSpatialBlob(pRecord, val.value);
		}

		virtual bool GetNull(const RecordData * pRecord) const = 0;
		/// any of the other Sets should reset the null status
		virtual void SetNull(Record *pRecord) const = 0;

		inline const GenericEngineBase * GetGenericEngine() const { return m_pGenericEngine; }
		void ReportFieldConversionError(const wchar_t *pMessage) const;
		inline bool IsReportingFieldConversionErrors() const { return m_pGenericEngine!=NULL; }

	};
}
