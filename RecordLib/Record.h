///////////////////////////////////////////////////////////////////////////////
//
// (C) 2005 SRC, LLC  -   All rights reserved
//
///////////////////////////////////////////////////////////////////////////////
//
// Module: RECORD.H
//
///////////////////////////////////////////////////////////////////////////////


#pragma once

#include <assert.h>
#include <vector>
#include <map>
#include <limits>

#include "FieldBase.h"

namespace SRC
{
	///////////////////////////////////////////////////////////////////////////////
	//	class MiniXmlParser
	class MiniXmlParser
	{
	public:
		struct TagInfo
		{
			friend class MiniXmlParser;
		private:
			WString m_strXml;
			struct Attribute
			{
				std::pair<const wchar_t *, const wchar_t *> name;
				std::pair<const wchar_t *, const wchar_t *> value;
			};
			mutable std::vector<Attribute> m_vAttributes;
			mutable bool m_bAttributeInit;

			const wchar_t *m_pXmlStart;
			const wchar_t *m_pXmlEnd;

			void InitAttributes() const;

		public:

			TagInfo(const wchar_t *pStart=NULL, const wchar_t *pEnd=NULL)
				: m_bAttributeInit(false)
				, m_pXmlStart(pStart)
				, m_pXmlEnd(pEnd)
			{
				if (m_pXmlStart!=NULL && pEnd==NULL)
					m_pXmlEnd = m_pXmlStart + wcslen(m_pXmlStart);
			}
			TagInfo(WString strXml)
				: m_strXml(strXml)
				, m_bAttributeInit(false)
			{
				m_pXmlStart = m_strXml.c_str();
				m_pXmlEnd = m_pXmlStart+m_strXml.Length();
			}

			inline TagInfo & operator = (WString strXml)
			{
				m_bAttributeInit = false;
				m_strXml = strXml;
				m_pXmlStart = m_strXml.c_str();
				m_pXmlEnd = m_pXmlStart+m_strXml.Length();
				return *this;
			}

			inline const wchar_t * Start() const { return m_pXmlStart; }
			inline const wchar_t * End() const { return m_pXmlEnd; }
		};
		static WString EscapeAttribute(WString strVal);
		static AString EscapeAttribute(AString strVal);
		inline static WString EscapeAttribute(const wchar_t *pVal) { return EscapeAttribute(WString(pVal)); }
		inline static AString EscapeAttribute(const char *pVal) { return EscapeAttribute(AString(pVal)); }
		static WString UnescapeAttribute(WString strVal, bool bAllowCdata = false);

		// Throwing an error when missing is a huge expense. Only do it if it is a disaster for it to be missing.
		static WString GetAttribute(const TagInfo &xmlTag, const wchar_t *pAttributeName, bool bThrowErrorIfMissing);

		// Returns true and sets value if the attribute is set; returns false and does not touch value if attr isn't set
		static bool GetAttribute(const TagInfo &xmlTag, const wchar_t *pAttributeName, WString& value);

		static bool FindXmlTag(const TagInfo &xmlTag, TagInfo &r_xmlElementTag, const wchar_t *pTagName);
		static bool FindFirstChildXmlTag(const TagInfo &xmlTag, TagInfo &r_xmlElementTag);
		static bool FindNextXmlTag(const TagInfo &xmlTag, TagInfo &r_xmlElementTag, const wchar_t *pTagName);
		static WString GetInnerXml(const TagInfo &xmlTag, bool bUnescapeValue = true, bool bTrim = true);
		static WString GetOuterXml(TagInfo xmlTag);

		static bool FindXmlTagWithAttribute(const TagInfo &xmlTag, TagInfo &r_xmlElementTag, const wchar_t *pTagName, const wchar_t *pAttributeName, const wchar_t *pAttributeValue);

		// these match the C# routines in SrcNetXmlUtil
		static bool GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, bool bDefault, bool bThrowErrorIfMissing=false);
		static int GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, int iDefault, bool bThrowErrorIfMissing=false);
		static __int64 GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, __int64 iDefault, bool bThrowErrorIfMissing=false);
		static double GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, double dDefault, bool bThrowErrorIfMissing=false);
		static WString GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, const wchar_t * strDefault, bool bThrowErrorIfMissing=false);

		// can't distinguish between not set and default value. Set to blank returns blank.
		static WString GetAttributeDefault(const TagInfo &xmlTag, const wchar_t *pAttributeName, WString strDefault);

		// can't distinguish between not set, set blank, and default value.
		// No validation on the value, it just calls _wtol on it.
		static int GetAttributeDefault(const TagInfo &xmlTag, const wchar_t *pAttributeName, int nDefault);

		// can't distinguish between not set, set blank, and default value. 
		// True is when the value is set and begins with 'T' or 't'. "Trumped up" is true. "Yes" and "1" are false.
		static bool GetAttributeDefault(const TagInfo &xmlTag, const wchar_t *pAttributeName, bool bDefault);
	};

	///////////////////////////////////////////////////////////////////////////////
	//	class Record
	class Record : public SmartPointerRefObj_Base
	{
		friend class RecordInfo;
		friend class RecordCopier;
		// m_pRecord contains:
		// The fixed data
		// and if there is potentially Var Data
		//	int varDataTotalLen
		// and for each varData
		//	int varDataLen
		//		varData
		mutable void *m_pRecord;

		unsigned m_nFixedRecordSize;
		bool m_bContainsVarData;

		unsigned m_nCurrentVarDataSize;
		unsigned m_nCurrentBufferSize;

		mutable bool m_bVarDataLenUnset;


		inline void Allocate(unsigned nNewMinimumVarDataSize)
		{
			unsigned nMinSize = (m_nFixedRecordSize + 4 + nNewMinimumVarDataSize);
			// assert(nMinSize<=MaxFieldLength); Don't need to assert, I am going to test
			
			if (nMinSize>MaxFieldLength)
			{
				if (nMinSize<MaxFieldLength64)
					throw Error("This file has large records that are not supported by the 32 bit version.  Use the 64 bit version instead.");
				else
					throw Error("Record too big:  Records are limited to " + AString().Assign(static_cast<__int64>(MaxFieldLength)) + " + bytes.");
			}

			if (m_nCurrentBufferSize<nMinSize)
			{
				if (m_bContainsVarData)
				{
					m_nCurrentBufferSize = nMinSize;
					m_nCurrentBufferSize *= 2;
					if (m_nCurrentBufferSize>MaxFieldLength)
						m_nCurrentBufferSize = MaxFieldLength;
				}
				else
					m_nCurrentBufferSize = m_nFixedRecordSize;

				// The following assert is a soft limit, not a hard one.
				// It is here to help find random uninitialized memory errors.
				//assert(m_nCurrentBufferSize>=0 && m_nCurrentBufferSize<0x2000000);
				m_pRecord = realloc(m_pRecord, m_nCurrentBufferSize);
				if (m_pRecord==0)
					throw Error("Unable to allocate " + AString().Assign(static_cast<__int64>(m_nCurrentBufferSize)) + " bytes of memory.");
			}
		}

	private:
		// this can only be created by CreateRecord in the RecordInfo
		inline Record()
			: m_pRecord(NULL)
			, m_nFixedRecordSize(0)
			, m_bContainsVarData(false)
			, m_nCurrentVarDataSize(0)
			, m_nCurrentBufferSize(0)
			, m_bVarDataLenUnset(false)
		{
		}

		inline void Init(int nFixedRecordSize, bool bContainsVarData)
		{
			m_nFixedRecordSize = nFixedRecordSize;
			m_bContainsVarData = bContainsVarData;
			//assert(m_pRecord==NULL);
			Allocate(0);
			Reset();
		}

	public:
		inline int GetVarDataSize()
		{
			return m_nCurrentVarDataSize;
		}
		inline void Reset(int nVarDataSize=0)
		{
			m_nCurrentVarDataSize = nVarDataSize;
			m_bVarDataLenUnset = true;
			assert(m_pRecord!=NULL);
		}
		
		inline ~Record()
		{
			if (m_pRecord)
				free(m_pRecord);
		}

		// returns the offset within the var data to the start
		inline int AddVarData(const void *pVarData, unsigned nLen)
		{
			if ((m_nFixedRecordSize + 4 + m_nCurrentVarDataSize + nLen)  > MaxFieldLength)
			{
				AString strError = "Record too big:  Records are limited to " + AString().Assign(static_cast<__int64>(MaxFieldLength)) + " + bytes.";
#if !defined(_WIN64) && !defined(__x86_64__)
				// SrcLib Replacement doesn't know how to convert unsigned values
				// but the field size limit should fit in a signed interger anyway
#ifndef __GNUG__
				static_assert(MaxFieldLength64<=INT_MAX, "MaxFieldLength64<=INT_MAX");
#endif
				strError += " In the 64bit version the limit would be " + AString().Assign(static_cast<__int64>(MaxFieldLength64)) + " + bytes.";
#endif
				throw Error(strError);
			}

			m_bVarDataLenUnset = true;

			// we return the actual offset in the vardata , including the vardata len.  That way, 0 can't happen
			int nRet = m_nCurrentVarDataSize+4;



			// if the vardata is shorter that 127 characters, only 1 byte will be used
			// for the length, setting the low bit to signal this condition
			// with the length be left shifted by 1 in either case
			// this would have to change for big endian processor architectures
			//int nLenLength;
			unsigned nStoreLen = nLen << 1;
			if (nLen>0x7f)
			{
				//nLenLength = sizeof(unsigned);
				Allocate(m_nCurrentVarDataSize+nLen+sizeof(unsigned));
				*reinterpret_cast<unsigned *>(static_cast<char *>(m_pRecord)+m_nFixedRecordSize+sizeof(m_nCurrentVarDataSize)+m_nCurrentVarDataSize) = nStoreLen;
				m_nCurrentVarDataSize += sizeof(unsigned);
			}
			else
			{
				//nLenLength = sizeof(unsigned char);
				nStoreLen |= 1;
				Allocate(m_nCurrentVarDataSize+nLen+sizeof(unsigned char));
				*reinterpret_cast<unsigned char *>(static_cast<char *>(m_pRecord)+m_nFixedRecordSize+sizeof(m_nCurrentVarDataSize)+m_nCurrentVarDataSize) = (unsigned char)nStoreLen;
				m_nCurrentVarDataSize += sizeof(unsigned char);
			}

			memcpy(static_cast<char *>(m_pRecord)+m_nFixedRecordSize+sizeof(m_nCurrentVarDataSize)+m_nCurrentVarDataSize, pVarData, nLen);
			m_nCurrentVarDataSize += nLen;
			return nRet;
		}

	
		inline void SetLength() const
		{
			m_bVarDataLenUnset = false;
			memcpy(static_cast<char *>(m_pRecord)+m_nFixedRecordSize, &m_nCurrentVarDataSize, sizeof(m_nCurrentVarDataSize));
		}

		inline RecordData * GetRecord()
		{ 
			if (m_bVarDataLenUnset && m_bContainsVarData)
				SetLength();

			return static_cast<RecordData *>(m_pRecord); 
		}
		inline const RecordData * GetRecord() const
		{ 
			if (m_bVarDataLenUnset && m_bContainsVarData)
				SetLength();

			return static_cast<const RecordData *>(m_pRecord); 
		}
	};


	///////////////////////////////////////////////////////////////////////////////
	//	class RecordInfo
	class RecordInfo
	{
		typedef std::vector<SmartPointerRefObj<FieldBase> > T_FieldVector;

		int m_nFixedRecordSize;
		bool m_bContainsVarData;
		T_FieldVector m_vFields;

		std::vector<WStringNoCase> m_vOriginalFieldNames;

		// Fast field name hash
		struct FieldNameUniquify
		{
			unsigned m_nHash;
			WStringNoCase m_strFieldName;

			inline void ComputeHash()
			{
				const wchar_t* p = m_strFieldName.c_str();
				unsigned len = m_strFieldName.Length();
				for (unsigned uPos = 0; uPos < len; uPos++)
				{
					m_nHash = (m_nHash >> (4)) ^ (m_nHash << 4) ^ towupper(*(p + uPos));
				}
			}

			inline FieldNameUniquify(WStringNoCase strFieldName)
				: m_nHash(0)
				, m_strFieldName(strFieldName)
			{
				ComputeHash();
			}

			inline FieldNameUniquify& operator=(WStringNoCase strFieldName)
			{
				m_strFieldName = strFieldName;
				ComputeHash();
				return *this;
			}

			inline bool operator<(const FieldNameUniquify& rhs) const
			{
				return (m_nHash == rhs.m_nHash ? m_strFieldName < rhs.m_strFieldName : m_nHash < rhs.m_nHash);
			}
		};
		
		std::map<FieldNameUniquify, unsigned> m_mapFieldNums;

		bool m_bLockIn;
		unsigned m_nMaxFieldLen;
		bool m_bStrictNaming;
		GenericEngineBase * m_pGenericEngineBase;
		WStringNoCase ValidateFieldName(WStringNoCase strFieldName, unsigned nFieldNum, bool bIssueWarnings = true);


	public:
		inline RecordInfo(unsigned nMaxFieldLen = 255, bool bStrictNaming = false, GenericEngineBase * pGenericEngineBase = NULL)
			: m_nFixedRecordSize(0)
			, m_bContainsVarData(false)
			, m_bLockIn(false)
			, m_nMaxFieldLen(nMaxFieldLen)
			, m_bStrictNaming(bStrictNaming)
			, m_pGenericEngineBase(pGenericEngineBase)
		{
		}

		// this will set the LockIn flag when producing the RecordXML and accept it when reading
		void SetLockIn(bool bLockIn = true) { m_bLockIn = bLockIn; }

		RecordInfo(RecordInfo &&o);
		inline RecordInfo(const RecordInfo &o) { *this = o; }
		RecordInfo & operator =(const RecordInfo &o);
		RecordInfo & operator =(RecordInfo &&o);

		void SetGenericEngine(GenericEngineBase * pGenericEngineBase);
		const GenericEngineBase * GetGenericEngine() const { return m_pGenericEngineBase; }

		inline unsigned NumFields() const { return unsigned(m_vFields.size()); }
		inline const FieldBase * operator [](size_t n) const { return m_vFields[n].Get(); }
		inline void ResetForLateRename(unsigned maxlen, bool bStrictNaming) 
		{ 
			m_nMaxFieldLen = maxlen; 
			m_bStrictNaming = bStrictNaming;
		}

		void SwapFieldNames(int nField1, int nField2);

		void AddField(SmartPointerRefObj<FieldBase> pField);
		const FieldBase *  AddField(const MiniXmlParser::TagInfo &tagField, const wchar_t *pNamePrefix = NULL);

		// this might give your suggestion of a name a number at the end if there is a conflict
		const FieldBase * RenameField(unsigned nField, WStringNoCase strNewName);
		const FieldBase * RenameField(WStringNoCase strOldFieldName, WStringNoCase strNewFieldName);

		static WString CreateFieldXml(WString strFieldName, E_FieldType ft, unsigned nSize=0, int nScale=0, WString strSource=L"", WString strDescription=L"");
		static WString GetFieldXml(const FieldBase &field, const wchar_t *pFieldNamePrefix = L"", const wchar_t *pFieldNameSuffix = L"", bool bIncludeSource = true);

		WString GetRecordXmlMetaData(bool bIncludeSource = true) const;

		void InitFromXml(const MiniXmlParser::TagInfo &xmlTag, const wchar_t *pNamePrefix = NULL, bool bIgnoreLockIn = false);

		SmartPointerRefObj<Record> CreateRecord() const;

		int GetFieldNum(WStringNoCase strField, bool bThrowError = true) const;
		inline const FieldBase * GetFieldByName(WStringNoCase strField, bool bThrowError = true) const
		{
			int nField = GetFieldNum(strField, bThrowError);
			if (nField<0)
				return NULL;
			else
				return m_vFields[nField].Get();
		}

		/*
		* GetFieldAndIndexByType
		* Find first field value by wanted field type,
		* if not found, and pi_bThrowOnError is true, 
		* throws an exception, otherwise returns <0, NULL>
		* if found, returns pair<idx, pFieldBase *>
		*/
		std::pair<unsigned, const SRC::FieldBase * > 
		GetFieldAndIndexByType(const SRC::E_FieldType pi_FieldType,
							   const bool pi_bThrowOnError = false, unsigned nSkipToIndex =0) const;

		/*
		* GetFieldsByType
		* Find all by wanted field type,
		*
		*  returns vector< pFieldBase *>
		*/
		std::vector< const SRC::FieldBase *  >
		GetFieldsByType(const SRC::E_FieldType pi_FieldType) const;

		/*
		* GetFieldNumByType
		* Returns index of first field, which is wanted type.
		* If not found, throws an exception.
		* pi_bThrowOnError is false, then returns 0 in this case
		*/
		inline unsigned long
		GetFieldNumByType(const SRC::E_FieldType pi_FieldType,
						  const bool pi_bThrowOnError = true) 
			const
		{
			return this->GetFieldAndIndexByType(pi_FieldType, pi_bThrowOnError).first;
		}

		/*
		* GetFieldByType
		* Returns first FieldBase *ptr, which is wanted type.
		* If not found, returns NULL or throws an exception if 
		* pi_bThrowOnError is true
		*/
		inline const SRC::FieldBase * 
		GetFieldByType(const SRC::E_FieldType pi_FieldType,
					   const bool pi_bThrowOnError = false) 
			const
		{
			return this->GetFieldAndIndexByType(pi_FieldType, pi_bThrowOnError).second;
		}


		unsigned GetNumFieldsByType(const SRC::E_FieldType pi_FieldType) const;

		bool operator==(const RecordInfo &o);

		// returns true if everything about the fields are the same
		// other than the names can be different
		bool EqualTypes(const RecordInfo &o, bool bAllowAdditionalFields = false) const;

#ifndef SRCLIB_REPLACEMENT
		// this returns a #.  If 2 records sets have the same # they should be binary compatible.
		unsigned GetHash();
#endif
		// returns nLen, pValue
		inline static BlobVal GetVarDataValue(const RecordData * pRec, int nFieldOffset);

		inline static void SetVarDataValue(Record *pRec, int nFieldOffset, unsigned nLen, const void *pValue);

		inline size_t GetRecordLen(const RecordData * pSrc) const;
		inline void AssertValid(const RecordData * pSrc) const;

		// copies a whole record to a memory buffer.
		// returns 0 if not enough room, numBytesUsed on success
		inline size_t Copy(void *pDest, size_t nAvailibleBytes, const RecordData * pSrc) const;

		// copys a whole record with an identical structure.  Use RecordCopier for more flexablility
		inline void Copy(Record *r_pRecordDest, const RecordData * pRecordSrc) const;

		// return is the record version #
		// 0 - pre 9.0
		// 1 - 9.0 (>256MB records)
		template <class TFile> unsigned Write(TFile &file, const RecordData * pRecord) const ;
		template <class TFile> void Read(TFile &file, Record *r_pRecord) const;
	};

	inline int RecordInfo::GetFieldNum(WStringNoCase strField, bool bThrowError /*= true*/) const
	{
		auto it = m_mapFieldNums.find(strField);
		if (it==m_mapFieldNums.end())
		{
			if (bThrowError)
				throw Error(L"The field \"" + strField + L"\" is not contained in the record.");
			else
				return -1;
		}
		return it->second;
	}

	inline /*static*/ BlobVal RecordInfo::GetVarDataValue(const RecordData * pRec, int nFieldOffset) 
	{
		unsigned nVarDataPos;
		memcpy(&nVarDataPos, ToCharP(pRec)+nFieldOffset, sizeof(nVarDataPos));

		BlobVal ret(0,NULL);
		// any values under 4 are pointing at invalid space and can be ignored
		// specifically a value of 1 indicates it is NULL
		if (nVarDataPos==0)
		{
			ret.nLength = 0;
			ret.pValue = L"";  // pointer to null, the field is empt, but not null
		}
		else if (nVarDataPos==1)
		{
			ret.nLength = 0;
			ret.pValue = NULL;  // a NULL value
		}
		// if the high bit is set, that means that this is an extended length - up to MaxFieldLength64 (2GB)
		// if not set, but there is a length in bits 29 & 30 then we have the smal string optimization
		else if ((nVarDataPos & 0x80000000)==0 && (nVarDataPos & 0x30000000)!=0)
		{
			// small string optimization
			// it is a 1,2 or 3 byte value packed into the len
			// this would have to change in a big endian architecture
			ret.nLength = nVarDataPos >> 28;
			ret.pValue = ToCharP(pRec)+nFieldOffset;
		}
		else
		{
			// the high bit may or may not be set.  Set only if it was bigger than MaxFieldLength32
			// not harm in stripping either way
			nVarDataPos &= 0x7fffffff;

//			assert(nVarDataPos>=4);
			memcpy(&ret.nLength, ToCharP(pRec)+nFieldOffset+nVarDataPos, sizeof(ret.nLength));

			// see comment in Record::AddVarData
			int nLenLength;
			if (ret.nLength & 1)
			{
				nLenLength = sizeof(unsigned char);
				ret.nLength &= 0xff;
			}
			else
				nLenLength = sizeof(unsigned);

			ret.nLength >>= 1;

			ret.pValue = ToCharP(pRec)+nFieldOffset+nVarDataPos + nLenLength;
		}
		return ret;
	}
	inline /*static*/ void RecordInfo::SetVarDataValue(Record *pRec, int nFieldOffset, unsigned nLen, const void *pValue) 
	{
		unsigned nVarDataPos = 0;
		// when len is 0, we don't need to put it in the var data at all
		if (nLen!=0)
		{
			// small string optimization
			// we want to pack strings smaller than 3 bytes into the 4 byte len
			if (nLen<=3)
			{
				// put the length in the 29th and 30th bits
				nVarDataPos = nLen << 28;
				// copy the data into the low order byes (up to 3 bytes) of the Pos
				// note this is relying on little endian.
				switch(nLen)
				{
				case 3:
					reinterpret_cast<unsigned char *>(&nVarDataPos)[2] = static_cast<const unsigned char *>(pValue)[2];
				case 2:
					reinterpret_cast<unsigned char *>(&nVarDataPos)[1] = static_cast<const unsigned char *>(pValue)[1];
				case 1:
					reinterpret_cast<unsigned char *>(&nVarDataPos)[0] = static_cast<const unsigned char *>(pValue)[0];
					break;
				}
			}
			else
			{
				nVarDataPos = pRec->AddVarData(pValue, nLen);

				nVarDataPos += (pRec->m_nFixedRecordSize-nFieldOffset);  // the pos is an offset from this field so we don't have to know the whole record layout

				// if the record is bigger than 256MB, it could interfere with the small string optimization above
				// so we set the high bit so it can be distinguished on the get
#ifndef __GNUG__
				static_assert(MaxFieldLength32==0x0fffffff, "MaxFieldLength32==0x0fffffff");
#endif
				if (nVarDataPos>MaxFieldLength32)
					nVarDataPos |= 0x80000000;
			}
		}
		else if (pValue==NULL)
			nVarDataPos = 1;  // this is a NULL value, not an empty one

		memcpy(reinterpret_cast<char *>(pRec->m_pRecord)+nFieldOffset, &nVarDataPos, sizeof(nVarDataPos));
	}

	inline size_t RecordInfo::GetRecordLen(const RecordData * pSrc) const
	{
		size_t nTotalSize = m_nFixedRecordSize;
		if (m_bContainsVarData)
		{
			// make sure to add the var data len field into the copy
			nTotalSize += sizeof(int);

			int nVarDataSize;
			memcpy(&nVarDataSize, ToCharP(pSrc)+m_nFixedRecordSize, sizeof(int));
			nTotalSize += nVarDataSize;
		}
		assert(nTotalSize<MaxFieldLength);
		return nTotalSize;
	}
	inline void RecordInfo::AssertValid(const RecordData * pSrc) const
	{
#ifdef _DEBUG
		GetRecordLen(pSrc);
#else
#ifndef __GNUG__
		pSrc;
#endif
#endif
	}

	inline size_t RecordInfo::Copy(void *pDest, size_t nAvailibleBytes, const RecordData * pSrc) const
	{
		size_t nTotalSize = GetRecordLen(pSrc);
		if (nAvailibleBytes<nTotalSize)
			return 0;

		memcpy(pDest, pSrc, nTotalSize);
		return nTotalSize;
	}
	inline void RecordInfo::Copy(Record *r_pRecordDest, const RecordData * pRecordSrc) const
	{
		r_pRecordDest->Reset();
		assert(r_pRecordDest->m_bContainsVarData==m_bContainsVarData);
		int nCopySize = m_nFixedRecordSize;
		if (m_bContainsVarData)
			nCopySize += sizeof(int);
		memcpy(r_pRecordDest->m_pRecord, pRecordSrc, nCopySize);
		if (m_bContainsVarData)
		{
			int nVarDataSize;
			memcpy(&nVarDataSize, ToCharP(pRecordSrc)+m_nFixedRecordSize, sizeof(int));
			r_pRecordDest->Allocate(nVarDataSize);
			memcpy(static_cast<char *>(r_pRecordDest->m_pRecord)+nCopySize, ToCharP(pRecordSrc)+nCopySize, nVarDataSize);
			r_pRecordDest->m_bVarDataLenUnset=false;
		}
	}

	// return is the record version #
	// 0 - pre 9.0
	// 1 - 9.0 (>256MB records)
	template <class TFile> unsigned RecordInfo::Write(TFile &file, const RecordData * pRecord) const
	{
		int nWriteSize = m_nFixedRecordSize;
		if (m_bContainsVarData)
			nWriteSize += sizeof(int);
		file.Write(pRecord, nWriteSize);
		if (m_bContainsVarData)
		{
			int nVarDataSize;
			memcpy(&nVarDataSize, ToCharP(pRecord)+m_nFixedRecordSize, sizeof(int));
			file.Write(ToCharP(pRecord)+m_nFixedRecordSize+sizeof(int), nVarDataSize);

#ifndef __GNUG__
			static_assert(MaxFieldLength32==0x0fffffff, "MaxFieldLength32==0x0fffffff");
#endif
			return (nWriteSize + nVarDataSize > 0x0fffffff) ? 1 : 0;
		}
		return 0;
	}
	template <class TFile> void RecordInfo::Read(TFile &file, Record *r_pRecord) const
	{
		r_pRecord->Reset();

		int nReadSize = m_nFixedRecordSize;
		if (m_bContainsVarData)
			nReadSize += sizeof(int);
		file.Read(r_pRecord->m_pRecord, nReadSize);
		if (m_bContainsVarData)
		{
			int nVarDataSize;
			memcpy(&nVarDataSize, static_cast<char *>(r_pRecord->m_pRecord)+m_nFixedRecordSize, sizeof(int));
			assert(nVarDataSize>=0);
			r_pRecord->Allocate(nVarDataSize+4);
			file.Read(static_cast<char *>(r_pRecord->m_pRecord)+m_nFixedRecordSize+sizeof(int), nVarDataSize);
			r_pRecord->m_bVarDataLenUnset=false;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//	class RecordCopier
	class RecordCopier : public SmartPointerRefObj_Base
	{
		const RecordInfo &m_recInfoSource;
		const RecordInfo &m_recInfoDest;
		bool m_bSuppressSizeOnlyConvErrors;

		std::vector<std::pair<int, int> > m_vDeferredAdds;
		void DoAdd(int nDestFieldNum, int nSourceFieldNum);

		struct CopyCmd
		{
			// if the field isn't changing its enough to know the positions
			int nSrcOffset;
			int nDestOffset;
			int nLen;

			// but if the data is changing, we need to do more...
			int nSrcFieldNum;
			int nDestFieldNum;

			bool bIsFieldChange;
			bool bIsVarData;
			unsigned nVarDataMaxBytes;

			inline bool operator <(const CopyCmd &o) const
			{
				return nSrcOffset<o.nSrcOffset;
			}
		};
		std::vector<CopyCmd> m_vCopyCmds;

		AString m_strATemp;
		WString m_strWTemp;
		RecordCopier(const RecordCopier &);
		RecordCopier & operator =(const RecordCopier &);
	public:
		// recInfoDest does not need to be complete until DoneAdding is called.  All adds are deferred until then
		RecordCopier(const RecordInfo &recInfoDest, const RecordInfo &recInfoSource, bool bSuppressSizeOnlyConvErrors = false)
			: m_recInfoSource(recInfoSource)
			, m_recInfoDest(recInfoDest)
			, m_bSuppressSizeOnlyConvErrors(bSuppressSizeOnlyConvErrors)
		{
		}

		void Add(int nDestFieldNum, int nSourceFieldNum);
		inline bool IsValid() const { return !m_vDeferredAdds.empty() || !m_vCopyCmds.empty(); }

		// recInfoDest MUST be complete before calling DoneAdding
		void DoneAdding();

		void Copy(Record *pRecDest, const RecordData * pRecSrc) const;

		void SetDestToNull(Record *pRecDest) const;
	};

}
