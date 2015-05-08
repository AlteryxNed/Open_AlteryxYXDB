///////////////////////////////////////////////////////////////////////////////
//
// (C) 2005 SRC, LLC  -   All rights reserved
//
///////////////////////////////////////////////////////////////////////////////
//
// Module: RECORD.CPP
//
///////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "Record.h"
#include "FieldTypes.h"
#include <algorithm>
#ifndef SRCLIB_REPLACEMENT
	#include "../../inc/blob.h"
	#include "../../inc/crc.h"
#endif

namespace SRC
{
	///////////////////////////////////////////////////////////////////////////////
	//	class MiniXmlParser

	WString EscapeAttributeOld(WString strVal)
	{
		return strVal.ReplaceString(L"&", L"&amp;").ReplaceString(L"\"", L"&quot;").ReplaceString(L">", L"&gt;").ReplaceString(L"<", L"&lt;").ReplaceString(L"'", L"&apos;");
	}
	AString EscapeAttributeOld(AString strVal)
	{
		return strVal.ReplaceString("&", "&amp;").ReplaceString("\"", "&quot;").ReplaceString(">", "&gt;").ReplaceString("<", "&lt;").ReplaceString("'", "&apos;");
	}

	template <class TChar> Tstr<TChar> TEscapeAttribute(Tstr<TChar> strVal)
	{
		if (strVal.IsEmpty())
			return strVal;
#ifdef SRCLIB_REPLACEMENT
		return EscapeAttributeOld(strVal);
#else
		const TChar *pInput = strVal.c_str();

		TFastOutBuffer<Tstr<TChar>, 1024> outBuffer;
		bool bChanged = false;
		const TChar *p = pInput;
		for (; *p!=0; ++p)
		{
			switch (*p)
			{
			case '&':
			case '\"':
			case '<':
			case '>':
			case '\'':
				bChanged = true;
				if (p!=pInput)
					outBuffer.Append(pInput, unsigned(p-pInput));
				pInput = p+1;
				outBuffer += '&';
				switch (*p)
				{
					case '&':
						outBuffer+= 'a';
						outBuffer += 'm';
						outBuffer += 'p';
						break;
					case '\"':
						outBuffer += 'q';
						outBuffer += 'u';
						outBuffer += 'o';
						outBuffer += 't';
						break;
					case '<':
						outBuffer += 'l';
						outBuffer += 't';
						break;
					case '>':
						outBuffer += 'g';
						outBuffer += 't';
						break;
					case '\'':
						outBuffer += 'a';
						outBuffer += 'p';
						outBuffer += 'o';
						outBuffer += 's';
						break;
				}
				outBuffer += ';';
			}
		}
		if (bChanged)
		{
			if (p!=pInput)
				outBuffer.Append(pInput, unsigned(p-pInput));
			return outBuffer.GetString();
		}
		else 
			return strVal;
#endif
	}

	WString MiniXmlParser::EscapeAttribute(WString strVal)
	{
#ifdef _DEBUG
		static bool bTested = false;
		if (!bTested)
		{
			const wchar_t * a_pTests[] = {
				L"123",
				L"1&23",
				L"&1&23",
				L"&1&2&3",
				L"&1&2&3&",
				L"1&\"<>\'23"
			};
			for (unsigned x=0; x<sizeofArray(a_pTests); ++x)
			{ 
				WString test = WString(a_pTests[x]);
				WString a = TEscapeAttribute(test);
				WString b = EscapeAttributeOld(test);
				assert(a==b);
			}
			bTested=true;
		}
#endif
		return TEscapeAttribute(strVal);
		//return strVal.ReplaceString(L"&", L"&amp;").ReplaceString(L"\"", L"&quot;").ReplaceString(L">", L"&gt;").ReplaceString(L"<", L"&lt;").ReplaceString(L"'", L"&apos;");
	}
	AString MiniXmlParser::EscapeAttribute(AString strVal)
	{
#ifdef _DEBUG
		static bool bTested = false;
		if (!bTested)
		{
			const char * a_pTests[] = {
				"123",
				"1&23",
				"&1&23",
				"&1&2&3",
				"&1&2&3&",
				"1&\"<>\'23"
			};
			for (unsigned x=0; x<sizeofArray(a_pTests); ++x)
				assert(TEscapeAttribute(AString(a_pTests[x]))==EscapeAttributeOld(AString(a_pTests[x])));
			bTested=true;
		}
#endif
		return TEscapeAttribute(strVal);
		//return strVal.ReplaceString("&", "&amp;").ReplaceString("\"", "&quot;").ReplaceString(">", "&gt;").ReplaceString("<", "&lt;").ReplaceString("'", "&apos;");
	}

	WString MiniXmlParser::UnescapeAttribute(WString strVal, bool bAllowCdata /*= false*/)
	{
		if (strVal.IsEmpty())
			return strVal;

		// if we don't need to unescape return string without copying
		unsigned nNoEscapeNeededLength = 0;
		const wchar_t * pSrcConst = strVal.c_str();
		unsigned nInputLength = strVal.Length();

		for ( ; nNoEscapeNeededLength < nInputLength ; ++nNoEscapeNeededLength)
		{
			wchar_t c = pSrcConst[nNoEscapeNeededLength];
			if (c==L'&' || (bAllowCdata && c==L'<' && wcsncmp(pSrcConst+1, L"![CDATA[", 8)==0))
				break;
		}

		if (nNoEscapeNeededLength == nInputLength)
			return strVal;

		wchar_t * pSrc = strVal.Lock() + nNoEscapeNeededLength;
		wchar_t * pDest = pSrc;
		while (wchar_t c = *pSrc)
		{
			if (bAllowCdata && c==L'<' && wcsncmp(pSrc+1, L"![CDATA[", 8)==0)
			{
				pSrc += 9;
				bool bFoundEndCdata = false;
				while (wchar_t c2 = *pSrc)
				{
					if (c2==L']' && pSrc[1]==L']' && pSrc[2]==L'>')
					{
						pSrc += 3;
						bFoundEndCdata = true;
						break;
					}
					else
					{
						*pDest++ = c2;
						pSrc++;
					}
				}
				if (!bFoundEndCdata)
					throw Error("An unclosed CDATA was found.");
			}
			else if (c==L'&')
			{
/*				if (wcsncmp(pSrc+1, L"amp;quot;", 9)==0)
				{
					*pDest++ = L'\"';
					pSrc += 10;
				}
				else if (wcsncmp(pSrc+1, L"amp;gt;", 7)==0)
				{
					*pDest++ = L'>';
					pSrc += 8;
				}
				else if (wcsncmp(pSrc+1, L"amp;lt;", 7)==0)
				{
					*pDest++ = L'<';
					pSrc += 8;
				}
				else*/ if (wcsncmp(pSrc+1, L"amp;", 4)==0)
				{
					*pDest++ = L'&';
					pSrc += 5;
				}
				else if (wcsncmp(pSrc+1, L"quot;", 5)==0)
				{
					*pDest++ = L'\"';
					pSrc += 6;
				}
				else if (wcsncmp(pSrc+1, L"apos;", 5)==0)
				{
					*pDest++ = L'\'';
					pSrc += 6;
				}
				else if (wcsncmp(pSrc+1, L"gt;", 3)==0)
				{
					*pDest++ = L'>';
					pSrc += 4;
				}
				else if (wcsncmp(pSrc+1, L"lt;", 3)==0)
				{
					*pDest++ = L'<';
					pSrc += 4;
				}
				else if (wcsncmp(pSrc+1, L"#xA;", 4)==0)
				{
					*pDest++ = L'\n';
					pSrc += 5;
				}
				else if (wcsncmp(pSrc+1, L"#xD;", 4)==0)
				{
					*pDest++ = L'\r';
					pSrc += 5;
				}
				else
				{
					*pDest++ = c;
					pSrc++;
				}
			}
			else
			{
				*pDest++ = c;
				pSrc++;
			}
		}
		*pDest = 0;
		strVal.Unlock();
		return strVal;

//		return strVal.ReplaceString(L"&amp;", L"&").ReplaceString(L"&quot;", L"\"").ReplaceString(L"&gt;", L">").ReplaceString(L"&lt;", L"<");
	}

	void MiniXmlParser::TagInfo::InitAttributes() const
	{
		if (!m_bAttributeInit)
		{
			m_vAttributes.clear();
			// the 1st attrubute can't start before the 4th char <a a="...
			for (const wchar_t *p = m_pXmlStart+3; *p!='>' && *p!='/' && p<m_pXmlEnd; p++)
			{
				wchar_t cPrev = p[-1];
				if (cPrev==' ' || cPrev=='\t' || cPrev=='\n')
				{
					wchar_t c = p[0];
					if (c!=' ' && c!='\t' && c!='\n')
					{
						// we have the start of an attribute
						TagInfo::Attribute att;
						att.name.first = p;
						att.name.second = p+1;
						for(;; ++att.name.second)
						{
							if (att.name.second==m_pXmlEnd)
								throw Error("Bad XML in GetAttribute");

							wchar_t c2 = *att.name.second;
							if (c2==' ' || c2=='\t' || c2=='\n' || c2=='=')
								break;
						}
						att.value.first = att.name.second+1;
						wchar_t cQuote;
						for(;; ++att.value.first)
						{
							if (att.value.first==m_pXmlEnd)
								throw Error("Bad XML in GetAttribute (2)");

							wchar_t c2 = att.value.first[-1];
							if (c2=='\"' || c2=='\'')
							{
								cQuote = c2;
								break;
							}
						}
						
						att.value.second = att.value.first;
						for(;; ++att.value.second)
						{
							if (att.value.second==m_pXmlEnd)
								throw Error("Bad XML in GetAttribute (3)");

							wchar_t c2 = *att.value.second;
							if (c2==cQuote)
								break;
						}

						m_vAttributes.push_back(att);
						p = att.value.second;
					}
				}
			}
			m_bAttributeInit = true;
			}
	}

	WString MiniXmlParser::GetAttribute(const TagInfo &xmlTag, const wchar_t *pAttributeName, bool bThrowErrorIfMissing)
	{
		if (!xmlTag.m_bAttributeInit)
			xmlTag.InitAttributes();
		
		int nLen = int(wcslen(pAttributeName));
		for (std::vector<TagInfo::Attribute>::const_iterator it = xmlTag.m_vAttributes.begin(); it != xmlTag.m_vAttributes.end(); ++it)
		{
			if (nLen==(it->name.second - it->name.first) && wcsncmp(pAttributeName, it->name.first, nLen)==0)
			{
				WString strRet;
				int nLen = int(it->value.second - it->value.first);
				if (nLen==0)
					return strRet;
				int nBytes = nLen*sizeof(*it->value.second);
				wchar_t *pBuffer = strRet.Lock(nLen);
				memcpy(pBuffer, it->value.first, nBytes);

				strRet.Unlock(nLen);
				return UnescapeAttribute(strRet);
			}
		}

		if (bThrowErrorIfMissing)
			throw SRC::Error(L"XmlParse Error: the attribute \""+WString(pAttributeName)+L"\" missing.");
		else
			return L"";
	}

	// Returns true and sets value if the attribute is set; returns false and does not touch value if attr isn't set
	/*static*/ bool MiniXmlParser::GetAttribute(const TagInfo &xmlTag, const wchar_t *pAttributeName, WString& value)
	{
		if (!xmlTag.m_bAttributeInit)
			xmlTag.InitAttributes();
		
		int nLen = int(wcslen(pAttributeName));
		for (const TagInfo::Attribute& attr: xmlTag.m_vAttributes) {
			if (nLen==(attr.name.second - attr.name.first) && wcsncmp(pAttributeName, attr.name.first, nLen)==0)
			{
				value.Assign(static_cast<const wchar_t*>(attr.value.first),int(attr.value.second-attr.value.first));
				value=UnescapeAttribute(std::move(value));
				return true;
			}
		}
		return false;
	}

	/*static*/ WString MiniXmlParser::GetAttributeDefault(const TagInfo &xmlTag, const wchar_t *pAttributeName, WString strDefault)
	{
		WString attrVal;
		if (GetAttribute(xmlTag,pAttributeName,attrVal))
			return attrVal;
		return strDefault;
	}

	/*static*/ int MiniXmlParser::GetAttributeDefault(const TagInfo &xmlTag, const wchar_t *pAttributeName, int nDefault)
	{
		WString strRet = GetAttribute(xmlTag, pAttributeName, false);
		if (strRet.IsEmpty())
			return nDefault;
		else
			return _wtol(strRet);
	}
	/*static*/ bool MiniXmlParser::GetAttributeDefault(const TagInfo &xmlTag, const wchar_t *pAttributeName, bool bDefault)
	{
		WString strRet = GetAttribute(xmlTag, pAttributeName, false);
		if (strRet.IsEmpty())
			return bDefault;
		else
			return 'T'==toupper(*strRet);
	}

	bool MiniXmlParser::FindXmlTag(const TagInfo &xmlTag, TagInfo &r_xmlElementTag, const wchar_t *pRequestedTagName)
	{
		if (xmlTag.m_pXmlStart==NULL || *xmlTag.m_pXmlStart==0)
			return false;

		r_xmlElementTag.m_bAttributeInit = false;

		r_xmlElementTag.m_pXmlStart = xmlTag.m_pXmlStart;
		
		WString strStartTag = L"<";
		String sTagName = L"";
		if (pRequestedTagName)
		{
			sTagName = pRequestedTagName;
			strStartTag += pRequestedTagName;	
		}
		else
		{
			r_xmlElementTag.m_pXmlStart ++;
		}
		
		for(;;)
		{
			r_xmlElementTag.m_pXmlStart = wcsstr(r_xmlElementTag.m_pXmlStart, strStartTag);
			if (!r_xmlElementTag.m_pXmlStart || (r_xmlElementTag.m_pXmlStart+strStartTag.Length())>=xmlTag.m_pXmlEnd)
				return false;

			if (sTagName == L"")
			{
				
				const wchar_t * pNameStart =  r_xmlElementTag.m_pXmlStart +1;
				unsigned nNameLen = 0;
				while (iswalnum(pNameStart[nNameLen]) || pNameStart[nNameLen]==L'_')
				{
					nNameLen++;
				}

				wchar_t * pNameBuff = sTagName.Lock( nNameLen+1);
				memcpy(pNameBuff,pNameStart,nNameLen* sizeof(wchar_t));
				pNameBuff[nNameLen] ='\0';
				sTagName.Unlock();
				strStartTag += sTagName;
				break;
			}

			wchar_t cNext = r_xmlElementTag.m_pXmlStart[strStartTag.Length()];
			if (iswalnum(cNext) || cNext==L'_')
				r_xmlElementTag.m_pXmlStart += strStartTag.Length();
			else 
				break;
		}

		r_xmlElementTag.m_pXmlEnd = r_xmlElementTag.m_pXmlStart+strStartTag.Length();

		// scan for the end of the tag
		wchar_t cCurrentQuote = 0;
		while(*r_xmlElementTag.m_pXmlEnd)
		{
			wchar_t c = *r_xmlElementTag.m_pXmlEnd;
			if (cCurrentQuote==0)
			{
				if (c=='\"' || c=='\'')
					cCurrentQuote = c;
				else if (c=='>')
						break;
			}
			else if (c==cCurrentQuote)
				cCurrentQuote = 0;

			++r_xmlElementTag.m_pXmlEnd;
		}

		if (!*r_xmlElementTag.m_pXmlEnd)
			throw SRC::Error(L"XmlParse Error: the element "+WString(sTagName)+L" is not properly formatted.");



		if (*(r_xmlElementTag.m_pXmlEnd-1)!='/')
		{
			r_xmlElementTag.m_pXmlEnd = r_xmlElementTag.m_pXmlStart+1;

			WString strCloseTag = WString(L"</") + sTagName;

			for(;;)
			{
				const wchar_t * pNextClose = wcsstr(r_xmlElementTag.m_pXmlEnd, strCloseTag);
				if (pNextClose == nullptr)
					throw Error(L"The tag \"" + String(pRequestedTagName) + L"\" is not properly closed.");
				while (isalnum(pNextClose[strCloseTag.Length()]) || pNextClose[strCloseTag.Length()]=='_' || pNextClose[strCloseTag.Length()]=='-' || pNextClose[strCloseTag.Length()]=='.')
					pNextClose = wcsstr(pNextClose+1, strCloseTag);
				if (!pNextClose)
					throw SRC::Error(L"XmlParse Error (1): the element "+WString(sTagName)+L" is not properly closed.");

				const wchar_t * pNextCloseEnd = pNextClose + strCloseTag.Length();
				while ((pNextCloseEnd<xmlTag.m_pXmlEnd) && *pNextCloseEnd!='>' && *pNextCloseEnd!=0)
					pNextCloseEnd++;
				
				if (!pNextCloseEnd)
					throw SRC::Error(L"XmlParse Error (1): the element "+WString(sTagName)+L" is not properly closed.");

				if ((pNextCloseEnd>=xmlTag.m_pXmlEnd) || *pNextCloseEnd!='>')
					throw SRC::Error(L"XmlParse Error (2): the element "+WString(sTagName)+L" is not properly closed.");

				pNextCloseEnd++; // increment past the >

				TagInfo tagEmbedded;
				// skip past any embedded tags
				if (FindXmlTag(TagInfo(r_xmlElementTag.m_pXmlEnd, pNextCloseEnd), tagEmbedded, sTagName) && tagEmbedded.m_pXmlStart<pNextClose)
				{
					r_xmlElementTag.m_pXmlEnd = tagEmbedded.m_pXmlEnd;
				}
				else
				{
					r_xmlElementTag.m_pXmlEnd = pNextCloseEnd;
					break;
				}
			}
		}
		else
			r_xmlElementTag.m_pXmlEnd++;  // move the end past the closing >
		if (r_xmlElementTag.m_pXmlEnd>xmlTag.m_pXmlEnd)
			throw SRC::Error(L"XmlParse Error (3): the element "+WString(sTagName)+L" is not properly closed.");
		return true;
	}

	/*static*/ bool MiniXmlParser::FindFirstChildXmlTag(const TagInfo &xmlTag, TagInfo &r_xmlElementTag)
	{
		return FindXmlTag(xmlTag, r_xmlElementTag, NULL);
	}
	
	/*static*/ bool MiniXmlParser::FindNextXmlTag(const TagInfo &xmlTag, TagInfo &r_xmlElementTag, const wchar_t *pTagName)
	{
		TagInfo xmlTagNew(xmlTag);
		xmlTagNew.m_pXmlStart = r_xmlElementTag.m_pXmlEnd;
		return FindXmlTag(xmlTagNew, r_xmlElementTag, pTagName);
	}

	WString MiniXmlParser::GetInnerXml(const TagInfo &xmlTag, bool bUnescapeValue, bool bTrim)
	{
		WString strRet;
		const wchar_t * pXmlStart = xmlTag.m_pXmlStart;

		// scan for the end of the tag
		while(*pXmlStart && *pXmlStart!=L'>')
			++pXmlStart;
		++pXmlStart;

		// scan to the beginning of the close
		const wchar_t * pXmlEnd = xmlTag.m_pXmlEnd - 1; // -1 because it always point 1 after the end of the tag
		while(pXmlEnd>pXmlStart && *pXmlEnd!=L'<')
			--pXmlEnd;

		if (*pXmlStart && pXmlStart<pXmlEnd)
		{
			int nLen = int(pXmlEnd-pXmlStart);
			wchar_t *pResult = strRet.Lock( nLen + 1);
			memcpy(pResult, pXmlStart, nLen*2);
			pResult[nLen] = 0;
			strRet.Unlock();
		}
		// we always trim 1st, because the a CDATA section could contain trailing spaces
		if (bTrim)
			strRet.Trim();
		if (bUnescapeValue)
			strRet = UnescapeAttribute(strRet, true);

		return strRet;
	}
	WString MiniXmlParser::GetOuterXml(TagInfo xmlTag)
	{
		WString strRet;
		if (xmlTag.m_pXmlStart!=NULL)
		{
			unsigned nLen = unsigned(xmlTag.m_pXmlEnd-xmlTag.m_pXmlStart);
			wchar_t *p = strRet.Lock(nLen+1);
			memcpy(p, xmlTag.m_pXmlStart, nLen*sizeof(wchar_t));
			p[nLen] = 0;
			strRet.Unlock();
		}
		return strRet;
	}

	/*static*/ bool MiniXmlParser::FindXmlTagWithAttribute(const TagInfo &xmlTag, TagInfo &r_xmlElementTag, const wchar_t *pTagName, const wchar_t *pAttributeName, const wchar_t *pAttributeValue)
	{
		TagInfo xmlTagStart(xmlTag);
		for (;;)
		{
			if (!FindXmlTag(xmlTagStart, r_xmlElementTag, pTagName))
				return false;
			if (GetAttribute(r_xmlElementTag, pAttributeName, false)==pAttributeValue)
				return true;
			xmlTagStart.m_pXmlStart = r_xmlElementTag.m_pXmlEnd;
		}
	}
	/*static*/ WString MiniXmlParser::GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, const wchar_t * strDefault, bool bThrowErrorIfMissing)
	{
		TagInfo xmlTag;
		if (!FindXmlTag(xmlTagParent, xmlTag, strElementName))
		{
			if (bThrowErrorIfMissing)
				throw Error(L"Missing value " + WString(strElementName) + L".");
			else
				return strDefault;
		}
		return GetInnerXml(xmlTag, true);
	}
	/*static*/ bool MiniXmlParser::GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, bool bDefault, bool bThrowErrorIfMissing)
	{
		TagInfo xmlTag;
		if (!FindXmlTag(xmlTagParent, xmlTag, strElementName))
		{
			if (bThrowErrorIfMissing)
				throw Error(L"Missing value " + WString(strElementName) + L".");
			else
				return bDefault;
		}
		String strValue = GetAttribute(xmlTag, L"value", bThrowErrorIfMissing);
		wchar_t c = *strValue.c_str();

		if (iswdigit(c) || c=='-')
			return 0.0!=_wtof(strValue);
		else
			return 'T'==toupper(c);
	}
	/*static*/ int MiniXmlParser::GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, int iDefault, bool bThrowErrorIfMissing)
	{
		TagInfo xmlTag;
		if (!FindXmlTag(xmlTagParent, xmlTag, strElementName))
		{
			if (bThrowErrorIfMissing)
				throw Error(L"Missing value " + WString(strElementName) + L".");
			else
				return iDefault;
		}
		return _wtoi(GetAttribute(xmlTag, L"value", bThrowErrorIfMissing));
	}
	/*static*/ __int64 MiniXmlParser::GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, __int64 iDefault, bool bThrowErrorIfMissing)
	{
		TagInfo xmlTag;
		if (!FindXmlTag(xmlTagParent, xmlTag, strElementName))
		{
			if (bThrowErrorIfMissing)
				throw Error(L"Missing value " + WString(strElementName) + L".");
			else
				return iDefault;
		}
		return _wtoi64(GetAttribute(xmlTag, L"value", bThrowErrorIfMissing));
	}
	/*static*/ double MiniXmlParser::GetFromXml(const TagInfo &xmlTagParent, const wchar_t * strElementName, double dDefault, bool bThrowErrorIfMissing)
	{
		TagInfo xmlTag;
		if (!FindXmlTag(xmlTagParent, xmlTag, strElementName))
		{
			if (bThrowErrorIfMissing)
				throw Error(L"Missing value " + WString(strElementName) + L".");
			else
				return dDefault;
		}
		return GetAttribute(xmlTag, L"value", bThrowErrorIfMissing).ConvertToDouble();
	}


	///////////////////////////////////////////////////////////////////////////////
	//	class RecordInfo
	RecordInfo::RecordInfo(RecordInfo &&o)
		: m_nFixedRecordSize(o.m_nFixedRecordSize)
		, m_bContainsVarData(o.m_bContainsVarData)
		, m_vFields(std::move(o.m_vFields))
		, m_vOriginalFieldNames(std::move(o.m_vOriginalFieldNames))
		, m_mapFieldNums(std::move(o.m_mapFieldNums))
		, m_nMaxFieldLen(o.m_nMaxFieldLen)
		, m_bStrictNaming(o.m_bStrictNaming)
		, m_pGenericEngineBase(o.m_pGenericEngineBase)
		, m_bLockIn(o.m_bLockIn)
	{
		o.m_nFixedRecordSize = 0;
		o.m_bContainsVarData = false;
		o.m_pGenericEngineBase = nullptr;
	}

	RecordInfo & RecordInfo::operator =(const RecordInfo &o)
	{
		m_nFixedRecordSize = o.m_nFixedRecordSize;
		m_bContainsVarData = o.m_bContainsVarData;
		m_vOriginalFieldNames = o.m_vOriginalFieldNames;
		m_pGenericEngineBase = o.m_pGenericEngineBase;

		m_vFields.clear();
		m_vFields.reserve(m_vFields.size());

		for(std::vector<SRC::SmartPointerRefObj<SRC::FieldBase> >::const_iterator it = o.m_vFields.begin(); it!=o.m_vFields.end(); ++it)
		{
			SRC::SmartPointerRefObj<SRC::FieldBase> p = (*it)->Copy();
			p->m_pGenericEngine = m_pGenericEngineBase;
			m_vFields.push_back(p);
		}

		m_mapFieldNums = o.m_mapFieldNums;
		m_nMaxFieldLen = o.m_nMaxFieldLen;
		m_bStrictNaming = o.m_bStrictNaming;
		m_bLockIn = o.m_bLockIn;

		return *this;
	}
	RecordInfo & RecordInfo::operator =(RecordInfo &&o)
	{
		m_nFixedRecordSize = o.m_nFixedRecordSize;
		m_bContainsVarData = o.m_bContainsVarData;
		m_vOriginalFieldNames = std::move(o.m_vOriginalFieldNames);
		m_pGenericEngineBase = o.m_pGenericEngineBase;

		m_vFields = std::move(o.m_vFields);
		m_mapFieldNums = std::move(o.m_mapFieldNums);
		m_nMaxFieldLen = o.m_nMaxFieldLen;
		m_bStrictNaming = o.m_bStrictNaming;
		m_bLockIn = o.m_bLockIn;

		o.m_nFixedRecordSize = 0;
		o.m_bContainsVarData = false;
		o.m_pGenericEngineBase = nullptr;

		return *this;
	}

	void RecordInfo::SetGenericEngine(GenericEngineBase * pGenericEngineBase)
	{
		m_pGenericEngineBase = pGenericEngineBase;
		for (std::vector<SRC::SmartPointerRefObj<FieldBase> >::iterator it = m_vFields.begin(); it != m_vFields.end(); it++)
			(*it)->m_pGenericEngine = m_pGenericEngineBase;
	}

	/*static*/ WString RecordInfo::CreateFieldXml(WString strFieldName, E_FieldType ft, unsigned nSize/*=0*/, int nScale/*=0*/, WString strSource/*=L""*/, WString strDescription/*=L""*/)
	{
#ifndef SRCLIB_REPLACEMENT
		typedef TFastOutBuffer<WString, 4096> StringBuffer;
#else
		typedef WString StringBuffer;
#endif

		WString strTemp;
		// the attributes have to be in alphebetical order to facilitate compared to parsed and rebuilt xml
		StringBuffer strRet;
		if (!strDescription.IsEmpty())
		{
			strRet.Append(L"\t<Field description=\"");
			strRet += MiniXmlParser::EscapeAttribute(strDescription);
			strRet.Append(L"\" name=\"");
		}
		else
			strRet.Append(L"\t<Field name=\"");

		strRet += MiniXmlParser::EscapeAttribute(strFieldName);

		if (ft==E_FT_FixedDecimal)
		{
			strRet.Append(L"\" scale=\"");
			strRet += strTemp.Assign(nScale);
		}

		// some places in code naively set the length to MaxFieldLength
		// even when it is a WString and the length is in characters, not bytes
		if (ft==E_FT_V_WString && nSize>MaxFieldLength/sizeof(wchar_t))
			nSize = MaxFieldLength/sizeof(wchar_t);
		switch (ft)
		{
		case E_FT_Bool:
		case E_FT_Byte:
		case E_FT_Int16:
		case E_FT_Int32:
		case E_FT_Int64:
		case E_FT_Float:
		case E_FT_Double:
		case E_FT_Date:
		case E_FT_Time:
		case E_FT_DateTime:
			break;			// these types don't have a size
		default:
			strRet.Append(L"\" size=\"");
			// SrcLib Replacement doesn't know how to convert unsigned values
			// but the field size limit should fit in a signed integer anyway
			assert(nSize<=static_cast<unsigned>(std::numeric_limits<int>::max()));
#ifndef __GNUG__
			static_assert(MaxFieldLength64<=INT_MAX, "MaxFieldLength64<=INT_MAX");
#endif
			strRet += strTemp.Assign(static_cast<int>(nSize));
		}

		if (!strSource.IsEmpty())
		{
			strRet.Append(L"\" source=\"");
			strRet += MiniXmlParser::EscapeAttribute(strSource);
		}
		strRet.Append(L"\" type=\"");
		strRet += GetNameFromFieldType(ft);
		strRet.Append(L"\"/>\n");
#ifndef SRCLIB_REPLACEMENT
		return strRet.GetString();
#else
		return strRet;
#endif
	}

	/*static*/ WString RecordInfo::GetFieldXml(const FieldBase &field, const wchar_t *pFieldNamePrefix/*=""*/, const wchar_t *pFieldNameSuffix/*=""*/, bool bIncludeSource /*= true*/)
	{
		return CreateFieldXml(pFieldNamePrefix + field.m_strFieldName + pFieldNameSuffix, field.m_ft, field.m_nSize, field.m_nScale, bIncludeSource ? field.GetSource() : L"", field.GetDescription());
	}

	WString RecordInfo::GetRecordXmlMetaData(bool bIncludeSource /*= true*/) const
	{
		WString strRet;
		if (m_bLockIn)
			strRet = L"<RecordInfo LockIn=\"True\" >\n";
		else
			strRet = L"<RecordInfo>\n";
		for (std::vector<SRC::SmartPointerRefObj<FieldBase> >::const_iterator it = m_vFields.begin(); it != m_vFields.end(); it++)
		{
			strRet += GetFieldXml(*it->Get(), L"", L"", bIncludeSource);
		}
		strRet += L"</RecordInfo>\n";
		return strRet;
	}

	std::pair<unsigned, const SRC::FieldBase * > RecordInfo::GetFieldAndIndexByType(const SRC::E_FieldType pi_FieldType,const bool pi_bThrowOnError /*= false*/, unsigned nSkipToIndex /*=0*/) const
	{
		const SRC::FieldBase *pField;
		unsigned idx = 0;
		T_FieldVector::const_iterator itRunner = m_vFields.begin();
		T_FieldVector::const_iterator itEnd = m_vFields.end();

		for ( ;idx < nSkipToIndex && itRunner != itEnd; ++idx, ++itRunner);

		for (  ; itRunner != itEnd; ++itRunner, ++idx) {
			pField = (*itRunner).Get();
			if (pField->m_ft == pi_FieldType) {
				return std::pair<unsigned, const SRC::FieldBase *>(idx, pField);
			}
		}
		if (pi_bThrowOnError) {
			throw SRC::Error("Field not found by wanted FieldType");
		}
		return std::pair<unsigned, const SRC::FieldBase *>(0, static_cast<const SRC::FieldBase *>(NULL));
	}

	std::vector< const SRC::FieldBase *  > RecordInfo::GetFieldsByType(const SRC::E_FieldType pi_FieldType) const
	{
		std::vector< const SRC::FieldBase * > aRet;
		std::pair<unsigned, const SRC::FieldBase * > nextInfo = GetFieldAndIndexByType(pi_FieldType);
		while(nextInfo.second != NULL)
		{
			aRet.push_back(nextInfo.second);
			nextInfo = GetFieldAndIndexByType(pi_FieldType,false,nextInfo.first+1);
		}
		return aRet;

	}


	unsigned RecordInfo::GetNumFieldsByType(const SRC::E_FieldType eType) const
	{
		unsigned nRet = 0;
		for (T_FieldVector::const_iterator itFields =  m_vFields.begin(); itFields != m_vFields.end(); itFields++)
		{
			if ((*itFields)->m_ft == eType)
				nRet++;
		}
		return nRet;
	}

	WStringNoCase RecordInfo::ValidateFieldName(WStringNoCase strFieldName, unsigned nFieldNum, bool bIssueWarnings /*= true*/)
	{
		WStringNoCase strFieldNameOrig = strFieldName;
		if (strFieldName.IsEmpty())
			strFieldName = L"Field_" + WString().Assign(int(NumFields() + 1));

		if (m_bStrictNaming && !iswalpha(*strFieldName) && *strFieldName!='_')
		{
			strFieldName = L"_" + strFieldName;
		}

		strFieldName.Truncate(m_nMaxFieldLen);
		if (m_bStrictNaming)
		{
			for (wchar_t * p = strFieldName.Lock(); *p; ++p)
			{
				if (*p>255 || !isalnum(*p))
					*p = L'_';
			}
			strFieldName.Unlock();
		}

		if (m_vOriginalFieldNames.size()<=nFieldNum)
			m_vOriginalFieldNames.resize(nFieldNum+1);
		m_vOriginalFieldNames[nFieldNum] = strFieldName;

		bool bFirst = true;
		int nNextNum = 1;
		WStringNoCase strTemp;
		bool bAutoRenamed = false;
		//Profiling showed that the find and insert below were pain points so FieldNameUniquify gives us a fast hash of the field name
		FieldNameUniquify fnu(strFieldName);
		for ( ; m_mapFieldNums.find(fnu)!=m_mapFieldNums.end() ; fnu = FieldNameUniquify(strFieldName))
		{
			if (bFirst)
			{
				bFirst = false;
				unsigned nPreviousField = m_mapFieldNums[fnu];
				// the original field name here conflicts with another, is it a rename?
				if (strFieldName!=m_vOriginalFieldNames[nPreviousField])
				{
					// it was a rename, since this is an original field name, it gets to keep it.
					m_mapFieldNums.insert(std::make_pair(fnu, nFieldNum));

					// and we rename the old field (again)
					m_vFields[nPreviousField]->SetFieldName(ValidateFieldName(m_vOriginalFieldNames[nPreviousField], nPreviousField, false));

					return strFieldName;
				}
			}
			/*
			while duplicate
				if (fiendnamelen>1 and 2nd to last char is not digit)
				{
					if last char is >=2 and <=8
					{
						increment digit
							goto done
					}
				}
				not done:
				if is last char digit
					append _
				append 2
				*/

			unsigned nFieldNumLen = strFieldName.Length();
			if (nFieldNumLen>=2 && !iswdigit(strFieldName[nFieldNumLen-2]) && 
				'2'<=strFieldName[nFieldNumLen-1] && strFieldName[nFieldNumLen-1]<='8')
			{
				strFieldName.Lock()[nFieldNumLen-1]++;
				strFieldName.Unlock();
			}
			else
			{
				if (iswdigit(strFieldName[nFieldNumLen-1]))
					strFieldName += '_';

				strFieldName += '2';
			}

			if (strFieldName.Length()>m_nMaxFieldLen)
			{
				// oops, we made a fieldname that was too long
				// we have to revert to a more ugly strategy
				String strNumber = String().Assign(++nNextNum);
				strFieldName.Truncate(m_nMaxFieldLen-strNumber.Length());
				strFieldName += strNumber;
			}

			bAutoRenamed = true;
		}


		if (bIssueWarnings && bAutoRenamed && m_pGenericEngineBase!=NULL)
		{
			String strMessage = L"There were multiple fields named \"";
			strMessage = strMessage + strFieldNameOrig.c_str();
			strMessage = strMessage + L"\".  The duplicate was renamed.";
			m_pGenericEngineBase->OutputMessage(GenericEngineBase::MT_Warning,  strMessage);
		}
		m_mapFieldNums.insert(std::make_pair(fnu, nFieldNum));
		return strFieldName;
	}

	void RecordInfo::AddField(SmartPointerRefObj<FieldBase> pField)
	{
		pField->SetFieldName(ValidateFieldName(pField->GetFieldName(), unsigned(m_vFields.size())));

		pField->m_nOffset = m_nFixedRecordSize;
		pField->m_pGenericEngine = m_pGenericEngineBase;
		m_vFields.push_back(pField);
		if (MaxFieldLength-pField->m_nRawSize < unsigned(m_nFixedRecordSize)) {
			__int64 tsize=static_cast<__int64>(m_nFixedRecordSize)+static_cast<__int64>(pField->m_nRawSize);
			String msg;
			if (tsize<MaxFieldLength64)
				msg=L"Record too big for for the 32 bit version. Use the 64 bit version instead: ";
			else
				msg=L"Record too big:  Records are limited to " + String().Assign(static_cast<__int64>(MaxFieldLength)) + L" bytes: ";
			msg += L"Trying to make a record with " + String().Assign(tsize) + L" bytes, when adding field named '";
			msg	+= pField->GetFieldName().c_str();
            msg += L"'";
			throw SRC::Error(msg);
		}
		m_nFixedRecordSize += pField->m_nRawSize;
		if (pField->m_bIsVarLength)
			m_bContainsVarData = true;
	}
	const FieldBase * RecordInfo::RenameField(unsigned nField, WStringNoCase strNewFieldName)
	{
		const FieldBase *pField = this->m_vFields[nField].Get();
		m_mapFieldNums.erase(pField->GetFieldName());
		pField->SetFieldName(ValidateFieldName(strNewFieldName, nField));
		return pField;
	}

	const FieldBase * RecordInfo::RenameField(WStringNoCase strOldFieldName, WStringNoCase strNewFieldName)
	{
		return RenameField(this->GetFieldNum(strOldFieldName), strNewFieldName);
	}
	

	void RecordInfo::SwapFieldNames(int nField1, int nField2)
	{
		const FieldBase *pField1 = this->m_vFields[nField1].Get();
		const FieldBase *pField2 = this->m_vFields[nField2].Get();
		std::swap(pField1->m_strFieldName, pField2->m_strFieldName);
	}


	inline const FieldBase * RecordInfo::AddField(const MiniXmlParser::TagInfo &tagField, const wchar_t *pNamePrefix /*= NULL*/)
	{
		WString strType = MiniXmlParser::GetAttribute(tagField, L"type", true);
		E_FieldType ft = GetFieldTypeFromName(strType);
		if (ft==E_FT_Unknown)
			throw Error(L"Unknown field type: " + strType);
		WString strName = MiniXmlParser::GetAttribute(tagField, L"name", true);
		if (pNamePrefix)
			strName = pNamePrefix + strName;
		WString strSize = MiniXmlParser::GetAttribute(tagField, L"size", false);
		int nSize = _wtol(strSize);
		if (nSize<=0 && IsString(ft))
			throw Error(L"Field: \"" + strName + L"\" is 0 length.");



		SRC::SmartPointerRefObj<FieldBase> pField;
		switch (ft)
		{
			case E_FT_Bool:
				pField = new Field_Bool(strName);
				break;
			case E_FT_Byte:
				pField = new Field_Byte(strName);
				break;
			case E_FT_Int16:
				pField = new Field_Int16(strName);
				break;
			case E_FT_Int32:
				pField = new Field_Int32(strName);
				break;
			case E_FT_Int64:
				pField = new Field_Int64(strName);
				break;
			case E_FT_FixedDecimal:
				{
					const wchar_t * pDecimal = wcschr(strSize, L'.');
					int nScale;
					if (pDecimal)
						nScale = _wtol(pDecimal+1);
					else
						nScale = _wtol(MiniXmlParser::GetAttribute(tagField, L"scale", true));
					if (nSize<=0)
						throw Error(L"Field: \"" + strName + L"\" is 0 length.");

					if (nScale>0 && nScale>(nSize-2))
						throw Error(L"Field: \"" + strName + L"\": \"" + strSize + L"\" has a to large a precision for its total size.  "
							L"Did you mean " + WString().Assign(nSize+nScale+3) + L"." + WString().Assign(nScale) + L"?");
					pField = new Field_FixedDecimal(strName, nSize, nScale);
					break;
				}
			case E_FT_Float:
				pField = new Field_Float(strName);
				break;
			case E_FT_Double:
				pField = new Field_Double(strName);
				break;
			case E_FT_String:
				// don't prevent this here, because it can break existing flows and files.
				if (m_pGenericEngineBase && nSize>static_cast<int>(MaxFixedLengthStringSize))
					m_pGenericEngineBase->OutputMessage(GenericEngineBase::MT_Warning, L"String fields are limited to " + WString().Assign(static_cast<__int64>(MaxFixedLengthStringSize)) + L" bytes.  use a V_String field instead.");

				pField = new Field_String(strName, nSize);
				break;
			case E_FT_WString:
				// don't prevent this here, because it can break existing flows and files.
				if (m_pGenericEngineBase && nSize>static_cast<int>(MaxFixedLengthStringSize))
					m_pGenericEngineBase->OutputMessage(GenericEngineBase::MT_Warning, L"WString fields are limited to " + WString().Assign(static_cast<__int64>(MaxFixedLengthStringSize)) + L" bytes.  use a V_WString field instead.");

				pField = new Field_WString(strName, nSize);
				break;
			case E_FT_V_String:
				// going from 64bit to 32 bit can leave fields set to more or less than the maximum size
				// clearly any size over this threashold was going for the max, so set it for max
				if (nSize>250000000)
					nSize = MaxFieldLength;

				pField = new Field_V_String(strName, nSize);
				m_bContainsVarData  = true;
				break;
			case E_FT_V_WString:
				if (nSize>125000000)
					nSize = MaxFieldLength/sizeof(wchar_t);
				pField = new Field_V_WString(strName, nSize);
				m_bContainsVarData  = true;
				break;
			case E_FT_Date:
				pField = new Field_Date(strName);
				break;
			case E_FT_Time:
				pField = new Field_Time(strName);
				break;
			case E_FT_DateTime:
				pField = new Field_DateTime(strName);
				break;
			case E_FT_Blob:
				pField = new Field_Blob(strName, false);
				m_bContainsVarData  = true;
				break;
			case E_FT_SpatialObj:
				pField = new Field_Blob(strName, true);
				m_bContainsVarData  = true;
				break;
			case E_FT_Unknown:
				break;
		}

		pField->SetSource(MiniXmlParser::GetAttribute(tagField, L"source", false));
		pField->SetDescription(MiniXmlParser::GetAttribute(tagField, L"description", false));

		AddField(pField);
		return pField.Get();
	}


	void RecordInfo::InitFromXml(const MiniXmlParser::TagInfo &xmlTag, const wchar_t *pNamePrefix /*= NULL*/, bool bIgnoreLockIn /* = false */)
	{
		if (xmlTag.Start()==NULL)
			return;

		MiniXmlParser::TagInfo tagRecordInfo;
		if (!MiniXmlParser::FindXmlTag(xmlTag, tagRecordInfo, L"RecordInfo"))
			return;
		if (!bIgnoreLockIn && m_bLockIn != MiniXmlParser::GetAttributeDefault(tagRecordInfo, L"LockIn", false))
		{
			if (m_bLockIn)
				throw Error("This tool can only be connected to another In-Database tool.");
			else
				throw Error("This tool is not compatible with an In-Database workflow.");
		}
		
		MiniXmlParser::TagInfo tagField;

		// this function can be additive (i.e. called more than once)
		// so it needs to maintain the offset between calls.
		// these vars are reset in the constructor
		//		m_nFixedRecordSize = 0;
		//		m_bContainsVarData = false;
		for (bool bFoundField = MiniXmlParser::FindXmlTag(tagRecordInfo, tagField, L"Field");
			bFoundField;
			bFoundField = MiniXmlParser::FindNextXmlTag(tagRecordInfo, tagField, L"Field"))
		{
			AddField(tagField, pNamePrefix);
		}
	}

	SmartPointerRefObj<Record> RecordInfo::CreateRecord() const
	{
		if (NumFields()==0)
			throw Error("RecordInfo::CreateRecord:  A record was created with no fields.");
		SmartPointerRefObj<Record> pRet(new Record);
		pRet->Init(m_nFixedRecordSize, m_bContainsVarData);
		return pRet;
	}

	bool RecordInfo::operator==(const RecordInfo &o)
	{
		if (m_vFields.size()!=o.m_vFields.size())
			return false;
		for (unsigned x=0; x<m_vFields.size(); x++)
		{
			if (!((*m_vFields[x].Get())==(*o.m_vFields[x].Get())))
				return false;
		}
		return true;
	}
	bool RecordInfo::EqualTypes(const RecordInfo &o, bool bAllowAdditionalFields /*= false*/) const
	{
		if (bAllowAdditionalFields)
		{
			if (m_vFields.size()>o.m_vFields.size())
				return false;
		}
		else
		{
			if (m_vFields.size()!=o.m_vFields.size())
				return false;
		}
		for (unsigned x=0; x<m_vFields.size(); x++)
		{
			if (!m_vFields[x]->EqualType(*o.m_vFields[x].Get()))
				return false;
		}
		return true;
	}

#ifndef SRCLIB_REPLACEMENT
	unsigned RecordInfo::GetHash()
	{
		BlobData blob;
		unsigned nNumFields = NumFields();
		blob.Add(&nNumFields, sizeof(nNumFields));
		for(unsigned x=0; x<nNumFields; ++x)
		{
			const FieldBase *pField = (*this)[x];
			blob.Add(&pField->m_ft, sizeof(pField->m_ft));
			blob.Add(&pField->m_nSize, sizeof(pField->m_nSize));
			blob.Add(&pField->m_nScale, sizeof(pField->m_nScale));
		}
		return CCRC32().FullCRC(blob.GetData(), unsigned(blob.GetSize()));
	}
#endif

	///////////////////////////////////////////////////////////////////////////////
	//	class RecordCopier
	void RecordCopier::Add(int nDestFieldNum, int nSourceFieldNum)
	{
		m_vDeferredAdds.push_back(std::pair<int, int>(nDestFieldNum, nSourceFieldNum));
	}
	void RecordCopier::DoAdd(int nDestFieldNum, int nSourceFieldNum)
	{
		const FieldBase & fieldSource = *m_recInfoSource[nSourceFieldNum];
		const FieldBase & fieldDest = *m_recInfoDest[nDestFieldNum];

		CopyCmd copyCmd;

		copyCmd.nSrcFieldNum = nSourceFieldNum;
		copyCmd.nDestFieldNum = nDestFieldNum;

		copyCmd.bIsFieldChange = fieldSource.m_ft != fieldDest.m_ft ||
			fieldSource.m_nRawSize != fieldDest.m_nRawSize ||
			fieldSource.m_nSize!=fieldDest.m_nSize ||
			(fieldSource.m_ft==E_FT_FixedDecimal && fieldSource.m_nScale!=fieldDest.m_nScale);


		if (!copyCmd.bIsFieldChange)
		{
			copyCmd.bIsFieldChange = false;
			copyCmd.nSrcOffset = fieldSource.GetOffset();
			copyCmd.nDestOffset = fieldDest.GetOffset();
			copyCmd.nLen = fieldSource.m_nRawSize;

			copyCmd.bIsVarData = fieldDest.m_bIsVarLength;
			// if the size is changing, we may need to truncate the data.
			if (copyCmd.bIsVarData)
				copyCmd.nVarDataMaxBytes = fieldDest.GetMaxBytes();
		}
		m_vCopyCmds.push_back(copyCmd);
	}

	void RecordCopier::DoneAdding()
	{
		for (std::vector<std::pair<int, int> >::const_iterator it = m_vDeferredAdds.begin(); it != m_vDeferredAdds.end(); it++)
		{
			DoAdd(it->first, it->second);
		}
		m_vDeferredAdds.clear();

		// the goal here is to merge together memcpy cmds
		// it only works it the fields are consequtive and identical
		std::sort(m_vCopyCmds.begin(), m_vCopyCmds.end());

		int prevIndex = 0;
		for (unsigned x=1; x<m_vCopyCmds.size(); ++x, ++prevIndex)
		{
			bool bCommandMerged = false;
			CopyCmd & thisCopyCmd = m_vCopyCmds[x];
			if (!thisCopyCmd.bIsVarData && !thisCopyCmd.bIsFieldChange)
			{
				CopyCmd & prevCopyCmd = m_vCopyCmds[prevIndex];
				if (!prevCopyCmd.bIsVarData && !prevCopyCmd.bIsFieldChange)
				{
					if (thisCopyCmd.nSrcOffset==(prevCopyCmd.nSrcOffset+prevCopyCmd.nLen) && thisCopyCmd.nDestOffset==(prevCopyCmd.nDestOffset+prevCopyCmd.nLen))
					{
						prevCopyCmd.nLen += thisCopyCmd.nLen;
						bCommandMerged = true;
						prevIndex--;
					}
				}
			}
			if (!bCommandMerged && static_cast<unsigned>(prevIndex) != x-1)
				m_vCopyCmds[prevIndex+1] = thisCopyCmd;
		}
		m_vCopyCmds.resize(prevIndex+1);
	}

	void RecordCopier::Copy(Record *pRecDest, const RecordData * pRecSrc) const
	{
		if (m_vDeferredAdds.size()!=0)
		{
			assert(false);
			const_cast<RecordCopier *>(this)->DoneAdding();
		}
		for (std::vector<CopyCmd>::const_iterator it = m_vCopyCmds.begin(); it!=m_vCopyCmds.end(); it++)
		{
			if (it->bIsFieldChange)
			{
				const FieldBase * pFieldDest = m_recInfoDest[it->nDestFieldNum];
				const FieldBase * pFieldSrc = m_recInfoSource[it->nSrcFieldNum];
				const GenericEngineBase * pSaveDestEngine = NULL;
				if (m_bSuppressSizeOnlyConvErrors && pFieldDest->m_ft==pFieldSrc->m_ft)
					pFieldDest->m_pGenericEngine = NULL;

				switch (pFieldDest->m_ft)
				{
					case E_FT_Bool:
						pFieldDest->SetFromBool(pRecDest, pFieldSrc->GetAsBool(pRecSrc));
						break;
					case E_FT_Byte:
					case E_FT_Int16:
					case E_FT_Int32:
						pFieldDest->SetFromInt32(pRecDest, pFieldSrc->GetAsInt32(pRecSrc));
						break;
					case E_FT_Int64:
						pFieldDest->SetFromInt64(pRecDest, pFieldSrc->GetAsInt64(pRecSrc));
						break;
					case E_FT_FixedDecimal:
						switch (pFieldSrc->m_ft)
						{
						case E_FT_Byte:
						case E_FT_Int16:
						case E_FT_Int32:
							pFieldDest->SetFromInt32(pRecDest, pFieldSrc->GetAsInt32(pRecSrc));
							break;
						case E_FT_Int64:
							pFieldDest->SetFromInt64(pRecDest, pFieldSrc->GetAsInt64(pRecSrc));
							break;
						case E_FT_Float:
						case E_FT_Double:
							pFieldDest->SetFromDouble(pRecDest, pFieldSrc->GetAsDouble(pRecSrc));
							break;
						default:
							pFieldDest->SetFromString(pRecDest, pFieldSrc->GetAsAString(pRecSrc));
							break;
						}
						break;
					case E_FT_Float:
					case E_FT_Double:
						pFieldDest->SetFromDouble(pRecDest, pFieldSrc->GetAsDouble(pRecSrc));
						break;
					case E_FT_WString:
					case E_FT_V_WString:
						pFieldDest->SetFromString(pRecDest, pFieldSrc->GetAsWString(pRecSrc));
						break;
					case E_FT_String:
					case E_FT_V_String:
					case E_FT_Date:
					case E_FT_Time:
					case E_FT_DateTime:
						pFieldDest->SetFromString(pRecDest, pFieldSrc->GetAsAString(pRecSrc));
						break;
					case E_FT_Blob:
						pFieldDest->SetFromBlob(pRecDest, pFieldSrc->GetAsBlob(pRecSrc));
						break;
					case E_FT_SpatialObj:
						pFieldDest->SetFromSpatialBlob(pRecDest, pFieldSrc->GetAsSpatialBlob(pRecSrc));
						break;
					case E_FT_Unknown:
						break;
				}
				if (m_bSuppressSizeOnlyConvErrors && pFieldDest->m_ft==pFieldSrc->m_ft)
					pFieldDest->m_pGenericEngine = pSaveDestEngine;

			}
			else if (it->bIsVarData)
			{
				const FieldBase * pFieldDest = m_recInfoDest[it->nDestFieldNum];
				const FieldBase * pFieldSrc = m_recInfoSource[it->nSrcFieldNum];
				BlobVal val = m_recInfoSource.GetVarDataValue(pRecSrc, pFieldSrc->GetOffset());

				// truncate the data if need be.
				unsigned nNewLen = unsigned(std::min(it->nVarDataMaxBytes, val.nLength));
				m_recInfoDest.SetVarDataValue(pRecDest, pFieldDest->GetOffset(), nNewLen, val.pValue);
			}
			else
			{
				// we can just copy a block of raw data over
				memcpy(static_cast<char *>(pRecDest->m_pRecord) + it->nDestOffset, ToCharP(pRecSrc)+it->nSrcOffset, it->nLen);
			}
		}
	}
	
	void RecordCopier::SetDestToNull(Record *pRecDest) const
	{
		for (unsigned x=0; x<m_recInfoDest.NumFields(); ++x)
			m_recInfoDest[x]->SetNull(pRecDest);

	}


}
