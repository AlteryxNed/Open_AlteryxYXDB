
#include "stdafx.h"

#include "Open_AlteryxYXDB.h"

#include "fcntl.h" // for open, error codes
#include "errno.h"
#include <sys/stat.h>

#ifdef __GNUG__
#include "unistd.h" // for close
#else
#include "io.h"
#endif


#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE


namespace Alteryx  { namespace OpenYXDB
{

	File_Large::File_Large()
		: m_iFileDescriptor(-1)
	{
	}

	File_Large::~File_Large()
	{
		Close();
	}

	void File_Large::Close()
	{
		if(m_iFileDescriptor!=-1)
		{
#ifdef __GNUG__
			close(m_iFileDescriptor);
#else
			_close(m_iFileDescriptor);
#endif
			m_iFileDescriptor = -1;
		}
	}

	void File_Large::OpenForRead(WString strFile)
	{
		m_strFile = strFile;

#ifdef __GNUG__
		AString astrFile = ConvertToAString(strFile);
		if(strchr(astrFile.c_str(), "?")!=nullptr)
			throw Error(L"Error in OpenForRead: Unicode filenames are not supported");

		m_iFileDescriptor = open(strFile, O_RDONLY | O_BINARY, 0);
#else
		m_iFileDescriptor = _wopen(strFile, _O_RDONLY | _O_BINARY);
#endif
		if(m_iFileDescriptor == -1)
		{
			File_Large::GetAndThrowError(L"Error in OpenForRead: ");
		}
	}

	void File_Large::OpenForWrite(WString strFile)
	{
		m_strFile = strFile;

#ifdef __GNUG__
		AString astrFile = ConvertToAString(strFile);
		if(strchr(astrFile.c_str(), "?")!=nullptr)
			throw Error(L"Error in OpenForRead: Unicode filenames are not supported");

		m_iFileDescriptor = open(strFile, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0);
#else
		m_iFileDescriptor = _wopen(strFile, _O_RDWR  | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE );
#endif

		if(m_iFileDescriptor == -1)
			File_Large::GetAndThrowError(L"Error in OpenForWrite: ");
	}


	__int64 File_Large::Tell() const
	{
		__int64 seekPos = 0;

#ifdef __GNUG__
		seekPos = lseek(m_iFileDescriptor, 0, SEEK_CUR);
#else
		seekPos = _lseeki64(m_iFileDescriptor, 0, SEEK_CUR);
#endif
		if(seekPos == -1)
			File_Large::GetAndThrowError(L"Error in Tell: ");

		return seekPos;
	}

	void File_Large::LSeek(__int64 nPos)
	{
		__int64 seekPos = 0;

#ifdef __GNUG__
		seekPos = lseek(m_iFileDescriptor, nPos, SEEK_SET);
#else
		seekPos = _lseeki64(m_iFileDescriptor, nPos, SEEK_SET);
#endif
		if(seekPos == -1)
			File_Large::GetAndThrowError(L"Error in LSeek: ");

	}

	unsigned File_Large::Read(void * _pBuffer, unsigned nNumBytesToRead)
	{
		unsigned ret = 0;

#ifdef __GNUG__
		ret = read(m_iFileDescriptor, _pBuffer, nNumBytesToRead);
#else
		ret = _read(m_iFileDescriptor, _pBuffer, nNumBytesToRead);
#endif

		if(ret != nNumBytesToRead)
			File_Large::GetAndThrowError(L"Error in Read: Unexpected number of bytes to read");

		return ret;	
	}

	unsigned File_Large::Write(const void * _pBuffer, unsigned nNumBytesToWrite)
	{
		unsigned ret = 0;
#ifdef __GNUG__
		ret = write(m_iFileDescriptor, _pBuffer, nNumBytesToWrite);
#else
		ret = _write(m_iFileDescriptor, _pBuffer, nNumBytesToWrite);
#endif
		if(ret != nNumBytesToWrite)
			File_Large::GetAndThrowError(L"Error in Write: ");

		return ret;
	}

	/*static*/ void File_Large::GetAndThrowError(WString strErrorIntro)
	{
		int nError = errno;

		WString errorMsg;
		ConvertString( errorMsg, strerror(nError) );
		errorMsg.Trim();
		throw Error(strErrorIntro + errorMsg);
	}

	Open_AlteryxYXDB::~Open_AlteryxYXDB()
	{
		try
		{
			Close();
		}
		catch (...)
		{
		}
	}
	
	/*virtual*/ void Open_AlteryxYXDB::Close()
	{
		if (m_pFile && m_pFile->IsOpen())
		{
			if (m_bCreateMode)
			{
				m_pCompressOutput->FlushBuffer();
				m_header.userHdr.nNumRecords = m_nCurrentRecord;

				// even if there is only 1 record this should be created
				//assert(m_vRecordBlockIndexPos.size()>0);
				m_header.userHdr.nRecordBlockIndexPos = m_pFile->Tell();
				m_header.userHdr.nCompressionVersion = 1;

				m_pFile->LSeek(0);
				m_pFile->Write(&m_header, sizeof(m_header));
				m_pFile->Close();
			}
			else
				m_pFile->Close();
			
			m_pFile.reset();
		}
	}

	/*virtual*/ void Open_AlteryxYXDB::Create(WString strFile, const wchar_t *pRecordInfoXml)
	{
		m_pFile.reset(new File_Large()); 
		m_pFile->OpenForWrite(strFile);

		m_bCreateMode = true;

		m_header.userHdr.nMetaInfoLen = wcslen(pRecordInfoXml)+1; // +1 to write the NULL terminator for convenience
		m_header.Write(*m_pFile);
		m_pFile->Write(pRecordInfoXml, (m_header.userHdr.nMetaInfoLen)*sizeof(wchar_t));

		m_pCompressOutput = new LZFBufferedOutput<File_Large *, GenericEngineBase >(this->m_recordInfo.GetGenericEngine(), m_pFile.get());

		m_recordInfo.InitFromXml(pRecordInfoXml);
		m_pRecord = m_recordInfo.CreateRecord();
	}

	/*virtual*/ void Open_AlteryxYXDB::AppendRecord(const RecordData *pRec)
	{
		if ((m_nCurrentRecord % RecordsPerBlock)==0)
		{
			m_pCompressOutput->FlushBuffer();
			m_vRecordBlockIndexPos.push_back(m_pFile->Tell());
		}

		m_recordInfo.Write(*m_pCompressOutput, pRec);
		m_nCurrentRecord++;
	}

	/*virtual*/ void Open_AlteryxYXDB::Open(WString strFile)
	{
		m_pFile.reset(new File_Large());
		m_pFile->OpenForRead(strFile);

		m_header.Read(*m_pFile);

		m_bIndexStartsBlock = (m_header.fileID & 0xff) == 3;
		if (m_header.userHdr.nCompressionVersion==1)
			m_pCompressInput = new LZFBufferedInput<File_Large * >(m_pFile.get());

		String strRecordInfoXml;
		wchar_t * pRecordInfoXml = strRecordInfoXml.Lock(m_header.userHdr.nMetaInfoLen);
		m_pFile->Read(pRecordInfoXml, m_header.userHdr.nMetaInfoLen*sizeof(wchar_t));
		strRecordInfoXml.Unlock();

		m_recordInfo.InitFromXml(strRecordInfoXml);
		m_pRecord = m_recordInfo.CreateRecord();

		// make sure we are at the first record in the file
		assert(	m_pFile->Tell() == int(sizeof(m_header) + m_header.userHdr.nMetaInfoLen*sizeof(wchar_t)));
	}
	
	/*virtual*/ WString Open_AlteryxYXDB::GetRecordXmlMetaData()
	{
		return m_recordInfo.GetRecordXmlMetaData();
	}

	/*virtual*/ const RecordData * Open_AlteryxYXDB::ReadRecord()
	{
		if (m_nCurrentRecord==m_header.userHdr.nNumRecords)
			return NULL;

		if ((m_nCurrentRecord % RecordsPerBlock)==0)
			GoBlockRecord(m_nCurrentRecord);

		m_nCurrentRecord++;
		Record * pRec = m_pRecord.Get();
		pRec->Reset();
			
		if (m_header.userHdr.nCompressionVersion==1)
			m_recordInfo.Read(*m_pCompressInput, pRec);
		else
			m_recordInfo.Read(*m_pFile, pRec);

		return pRec->GetRecord();
	}

	/*virtual*/ __int64 Open_AlteryxYXDB::GetNumRecords()
	{
		return m_header.userHdr.nNumRecords;
	}


	void Open_AlteryxYXDB::GoBlockRecord(__int64 nRecord)
	{
		if (nRecord==0)
		{
			m_pFile->LSeek(sizeof(m_header) + m_header.userHdr.nMetaInfoLen*sizeof(wchar_t));
			if (m_pCompressInput.Get())
				m_pCompressInput->Reset();
			m_nCurrentRecord = 0;
		}
		else
		{
			if (m_vRecordBlockIndexPos.size()==0)
			{
				m_pFile->LSeek(m_header.userHdr.nRecordBlockIndexPos);
				unsigned nNewArraySize=0;
				m_pFile->Read(&nNewArraySize, sizeof(nNewArraySize));
				m_vRecordBlockIndexPos.resize(nNewArraySize);
				
				m_pFile->Read(&*m_vRecordBlockIndexPos.begin(), nNewArraySize*sizeof(__int64));
			}
			__int64 nNewPos = m_vRecordBlockIndexPos[unsigned(nRecord/RecordsPerBlock)];
			m_pFile->LSeek(nNewPos);
			if (m_pCompressInput.Get())
				m_pCompressInput->Reset();
		}
	}

	/*virtual*/ void Open_AlteryxYXDB::GoRecord(__int64 nRecord /*= 0*/)
	{
		if (nRecord>=m_header.userHdr.nNumRecords || nRecord<0)
			throw Error(L"Open_AlteryxYXDB::GoRecord: Attempt to seek past the end of the file");
		else if (nRecord==m_nCurrentRecord)
			; // do nothing
		else
		{
			unsigned numSkipRecs = 0;
			if (nRecord>m_nCurrentRecord && nRecord<(m_nCurrentRecord + RecordsPerBlock - (m_nCurrentRecord % RecordsPerBlock)))
				numSkipRecs = unsigned(nRecord-m_nCurrentRecord);
			else
			{
				// read record will reset to the correct spot when it tries to read the next record
				numSkipRecs = unsigned(nRecord % RecordsPerBlock);
				m_nCurrentRecord = nRecord-numSkipRecs;
			}

			for (unsigned x=0; x<numSkipRecs; x++)
				ReadRecord();
		}
	}

}}
