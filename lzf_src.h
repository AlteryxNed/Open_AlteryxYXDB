///////////////////////////////////////////////////////////////////////////////
//
// (C) 2008 SRC, LLC  -   All rights reserved
//
///////////////////////////////////////////////////////////////////////////////
//
// Module: LZF_SRC.H
//
///////////////////////////////////////////////////////////////////////////////


#ifndef __LZF_SRC_H__
#define __LZF_SRC_H__
//#include <srclib.h>
extern "C"
{
	#include "lzf.h"
}

namespace SRC
{
	// TFileP is usually a SmartPointer to a file
	template <class TFileP, class TEngine, unsigned BufferSize=0x40000> class LZFBufferedOutput : public SmartPointerRefObj_Base
	{
		struct CompressBuffer
		{
			unsigned char m_pOutBuffer[BufferSize];
			unsigned m_nOutBufferUsed;
			unsigned char m_pInBuffer[BufferSize];
			unsigned m_nInBufferUsed;

			TFileP m_pFile;
#ifndef __GNUC__
			HANDLE m_hEvent;
#endif
			inline CompressBuffer(TFileP pFile, bool bCreateEvent)
				: m_nOutBufferUsed(0)
				, m_nInBufferUsed(0)
				, m_pFile(pFile)
			{
#ifndef __GNUC__
				if (bCreateEvent)
					m_hEvent = CreateEvent(NULL, false, false, NULL);
				else
					m_hEvent = NULL;
#endif
			}
			
			inline ~CompressBuffer()
			{
#ifndef __GNUC__
				if (m_hEvent!=NULL)
					CloseHandle(m_hEvent);
#endif
			}

			void DoCompress()
			{
				assert(m_nInBufferUsed!=0);
				m_nOutBufferUsed = lzf_compress(m_pInBuffer, m_nInBufferUsed, m_pOutBuffer, m_nInBufferUsed-1);
			}

			// this will be called in a worker thread
			// hence it cannot modify m_nInBufferUsed because
			// we use that in the master thread to know if anything happened
			static void __stdcall DoCompress(void *_pThis)
			{
				CompressBuffer *pThis = static_cast<CompressBuffer *>(_pThis);
				pThis->DoCompress();
#ifndef __GNUC__
				SetEvent(pThis->m_hEvent);
#endif
			}

#ifndef __GNUC__
			inline void WaitForCompletion()
			{
				WaitForSingleObject(m_hEvent, INFINITE);
			}
#endif
			// this should always be called from the master thread
			inline void DoWrite()
			{
				assert(m_nInBufferUsed!=0);

				// it wasn't able to compress, just write the data straight out
				if (m_nOutBufferUsed==0)
				{
					assert(m_nInBufferUsed<=BufferSize);
					unsigned nResultBytes = m_nInBufferUsed;
					if (BufferSize<=0x7fff)
					{
						unsigned short snResultBytes = static_cast<unsigned short>(nResultBytes | 0x8000);
						m_pFile->Write(&snResultBytes, sizeof(snResultBytes));
					}
					else
					{
						nResultBytes |= 0x80000000;
						m_pFile->Write(&nResultBytes, sizeof(nResultBytes));
					}
					m_pFile->Write((const char *)m_pInBuffer, m_nInBufferUsed);
				}
				else
				{
					assert(m_nOutBufferUsed<=BufferSize);
					if (BufferSize<=0x7fff)
					{
						unsigned short snResultBytes = static_cast<unsigned short>(m_nOutBufferUsed);
						m_pFile->Write(&snResultBytes, sizeof(snResultBytes));
					}
					else
						m_pFile->Write(&m_nOutBufferUsed, sizeof(m_nOutBufferUsed));
					m_pFile->Write((const char *)m_pOutBuffer, m_nOutBufferUsed);
				}
				m_nInBufferUsed = 0;
				m_nOutBufferUsed = 0;
			}
		};

		inline void DoWrite(CompressBuffer *pCurrentBuffer)
		{
			try
			{
				pCurrentBuffer->DoWrite();
			}
			catch (...)
			{
				pCurrentBuffer->m_nInBufferUsed = 0;
				pCurrentBuffer->m_nOutBufferUsed = 0;

				// go through all the buffers and flush them WITHOUT Writing, starting with the next one.
				if (m_pNextBuffer->m_nInBufferUsed!=0)
				{
#ifndef __GNUC__
					m_pNextBuffer->WaitForCompletion();
#endif
					m_pNextBuffer->m_nInBufferUsed = 0;
					m_pNextBuffer->m_nOutBufferUsed = 0;
				}
				if (m_pCurrentBuffer->m_nInBufferUsed!=0)
				{
#ifndef __GNUC__
					m_pCurrentBuffer->WaitForCompletion();
#endif
					m_pCurrentBuffer->m_nInBufferUsed = 0;
					m_pCurrentBuffer->m_nOutBufferUsed = 0;
				}
				throw;
			}
		}


		CompressBuffer m_buffer1;
		// this isn't always used, so we don't allocate it until we need it.
		std::auto_ptr<CompressBuffer> m_pBuffer2;
		CompressBuffer *m_pCurrentBuffer;
		CompressBuffer *m_pNextBuffer;

		const TEngine *m_pEngine;

	public:
		LZFBufferedOutput(const TEngine *pEngine, TFileP pFile);
		inline void FlushBuffer();
		~LZFBufferedOutput();
		void Write(const void *pBuffer, unsigned nSize);
	};

	template <class TFileP, class TEngine, unsigned BufferSize> LZFBufferedOutput<TFileP, TEngine, BufferSize>::LZFBufferedOutput(const TEngine *pEngine, TFileP pFile)
		: m_buffer1(pFile, pEngine!=NULL)
		, m_pEngine(pEngine)
	{
		m_pCurrentBuffer = &m_buffer1;
		if (m_pEngine)
		{
			m_pBuffer2.reset(new CompressBuffer(pFile, pEngine!=NULL));
			m_pNextBuffer = m_pBuffer2.get();
		}
		else
			m_pNextBuffer = NULL;
	}

	template <class TFileP, class TEngine, unsigned BufferSize> void LZFBufferedOutput<TFileP, TEngine, BufferSize>::FlushBuffer()
	{
#ifndef __GNUC__
		if (m_pEngine)
		{
			// queue up compressing this buffer so it can happen while we are writing the previos data
			if (m_pCurrentBuffer->m_nInBufferUsed!=0)
				m_pEngine->QueueThread(CompressBuffer::DoCompress, m_pCurrentBuffer);
			
			// the Next in this case might actually be the previous so we have to write it 1st
			// if it has no data, the write does nothing
			if (m_pNextBuffer->m_nInBufferUsed!=0)
			{
				m_pNextBuffer->WaitForCompletion();
				DoWrite(m_pNextBuffer);
			}
			if (m_pCurrentBuffer->m_nInBufferUsed!=0)
			{
				m_pCurrentBuffer->WaitForCompletion();
				DoWrite(m_pCurrentBuffer);
			}
		}
		else
#endif
		{
			// single threaded mode
			if (m_pCurrentBuffer->m_nInBufferUsed!=0)
			{
				m_pCurrentBuffer->DoCompress();
				DoWrite(m_pCurrentBuffer);
			}
		}
	}

	template <class TFileP, class TEngine, unsigned BufferSize> void LZFBufferedOutput<TFileP, TEngine, BufferSize>::Write(const void *pBuffer, unsigned nSize)
	{
		while (nSize>0)
		{
			unsigned nCopySize = std::min(BufferSize-m_pCurrentBuffer->m_nInBufferUsed, nSize);
			memcpy(m_pCurrentBuffer->m_pInBuffer+m_pCurrentBuffer->m_nInBufferUsed, pBuffer, nCopySize);
			m_pCurrentBuffer->m_nInBufferUsed += nCopySize;
			nSize -= nCopySize;
			pBuffer = ((char *)pBuffer) + nCopySize;

			if (m_pCurrentBuffer->m_nInBufferUsed==BufferSize)
			{
#ifndef __GNUC__
				if (m_pEngine)
				{
					// queue up the data for compressing
					m_pEngine->QueueThread(CompressBuffer::DoCompress, m_pCurrentBuffer);
					
					// switch to the next buffer that is hopefully already done compressing
					std::swap(m_pCurrentBuffer, m_pNextBuffer);
					
					// and write any previously compressed data in this buffer
					// the DoWrite will take care of waiting for the compression to be done if needed
					if (m_pCurrentBuffer->m_nInBufferUsed!=0)
					{
						m_pCurrentBuffer->WaitForCompletion();
						DoWrite(m_pCurrentBuffer);
					}
				}
				else
#endif
				{
					// single threaded mode
					m_pCurrentBuffer->DoCompress();
					DoWrite(m_pCurrentBuffer);
				}
			}
		}
	}

	template <class TFileP, class TEngine, unsigned BufferSize> LZFBufferedOutput<TFileP, TEngine, BufferSize>::~LZFBufferedOutput()
	{
		FlushBuffer();
	}

	////////////////////////////////////////////////////////////////////////////////
	// class LZFBufferedOutput
	// TFileP is usually a SmartPointer to a file
	template <class TFileP, unsigned BufferSize=0x40000> class LZFBufferedInput : public SmartPointerRefObj_Base
	{
		unsigned char m_pOutBuffer[BufferSize];
		unsigned char m_pInBuffer[BufferSize];
		unsigned nInBufferNext;
		unsigned nInBufferSize;

		TFileP m_pFile;

	public:
		LZFBufferedInput(TFileP pFile);
		void Reset()
		{
			nInBufferSize = 0;
			nInBufferNext = 0;
		}
		unsigned Read(void *pBuffer, unsigned nSize);

		TFileP GetFile() { return m_pFile;}
	};
	template <class TFileP, unsigned BufferSize> LZFBufferedInput<TFileP, BufferSize>::LZFBufferedInput(TFileP pFile)
		: nInBufferNext(0), nInBufferSize(0)
	{
		m_pFile = pFile;
	}

	template <class TFileP, unsigned BufferSize> unsigned LZFBufferedInput<TFileP, BufferSize>::Read(void *pBuffer, unsigned nSize)
	{
		unsigned nRet = nSize;
		while (nSize>0)
		{
			if (nInBufferSize<=nInBufferNext)
			{
				unsigned nResultBytes = 0;
				bool bUncompressed = false;
				if (BufferSize<=0x7fff)
				{
					unsigned short snResultBytes = static_cast<unsigned short>(nResultBytes);
					unsigned nBytesRead = m_pFile->Read(&snResultBytes, sizeof(snResultBytes));
					if (nBytesRead==0)
						return nRet-nSize;  // EOF
					if (sizeof(snResultBytes)!=nBytesRead)
						throw Error("Internal Error in LZFBufferedInput<TFileP, BufferSize>::Read");
					if (snResultBytes & 0x8000)
					{
						snResultBytes &= 0x7fff;
						bUncompressed = true;
					}
					nResultBytes = snResultBytes;
				}
				else
				{
					m_pFile->Read(&nResultBytes, sizeof(nResultBytes));
					if (nResultBytes & 0x80000000)
					{
						nResultBytes &= 0x7fffffff;
						bUncompressed = true;
					}
				}

				if (bUncompressed)
				{
					nInBufferSize = nResultBytes;
					if (nInBufferSize>sizeof(m_pOutBuffer))
					{
						assert(false);
						throw Error("Internal Error in LZFBufferedInput<TFileP>::Read: corrupt file.");
					}

					if (nInBufferSize!=m_pFile->Read(m_pOutBuffer, nInBufferSize))
						throw Error("Internal Error in LZFBufferedInput<TFileP>::Read: Not enough bytes read");
				}
				else
				{
					size_t nBytesRead = m_pFile->Read(m_pInBuffer, nResultBytes);
					nInBufferSize = lzf_decompress(m_pInBuffer, unsigned(nBytesRead), m_pOutBuffer, unsigned(sizeof(m_pOutBuffer)));
					if (nInBufferSize == 0)
						return 0;  // EOF - may get in infinite loop
				}
				nInBufferNext = 0;
			}
			unsigned nCopySize = std::min(unsigned(nInBufferSize-nInBufferNext), nSize);
			memcpy(pBuffer, m_pOutBuffer+nInBufferNext, nCopySize);
			nInBufferNext += nCopySize;
			nSize -= nCopySize;
			pBuffer = ((char *)pBuffer) + nCopySize;
		}
		return nRet;
	}
}


#endif //__LZF_SRC_H__