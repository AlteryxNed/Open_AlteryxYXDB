
#include "stdafx.h"
#include "Open_AlteryxYXDB.h"
#include <iostream>
#include <random>


SRC::AString EnglishNumber(int n) 
{
	const static int numbers[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,30,40,50,60,70,80,90,100,1000,1000000,1000000000};
	const static SRC::AString english[] = {"zero", "one", "two", "three", "four", "five", "six","seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", "fourteen","fifteen", "sixteen", "seventeen", "eighteen", "nineteen", "twenty","thirty", "fourty", "fifty", "sixty", "seventy", "eighty", "ninety","hundred", "thousand", "million", "billion"};

	SRC::AString returnString;

	if (n<0)
	{
		returnString = "negative";
		n = -n;
	}

	for(int i = sizeof(numbers)/sizeof(*numbers) - 1; i >= 0; i--) 
	{
		if(n == numbers[i])
		{
			if (!returnString.empty())
				returnString += " ";
			if (n>=100)
			{
				returnString += english[1];
				returnString += " ";
			}

			returnString += english[i];
			break;
		}
		else if (n>numbers[i])
		{
			if(!returnString.empty())
				returnString += " ";

			if(n >= 100)
			{
				returnString += EnglishNumber(n / numbers[i]);
				returnString += " ";
			}

			returnString += english[i];
			n -= (n / numbers[i]) * numbers[i];
		}
	}

	return returnString;
}
int _tmain(int argc, _TCHAR* argv[])
{
	try
	{

		{
			SRC::RecordInfo recordInfoOut;
			recordInfoOut.AddField(SRC::RecordInfo::CreateFieldXml(L"Number", SRC::E_FT_Int32));
			recordInfoOut.AddField(SRC::RecordInfo::CreateFieldXml(L"English", SRC::E_FT_V_String, 256));
			Alteryx::OpenYXDB::Open_AlteryxYXDB fileOut;
			fileOut.Create(L"temp.yxdb", recordInfoOut.GetRecordXmlMetaData());
			SRC::SmartPointerRefObj<SRC::Record> pRec = recordInfoOut.CreateRecord();
			std::mt19937 r;
			for (unsigned x=0; x<100; ++x)
			{
				pRec->Reset();
				int v = r();
				recordInfoOut[0]->SetFromInt32(pRec.Get(), v);
				recordInfoOut[1]->SetFromString(pRec.Get(), EnglishNumber(v));
				fileOut.AppendRecord(pRec->GetRecord());
			}
		}


		Alteryx::OpenYXDB::Open_AlteryxYXDB file;
		file.Open(L"temp.yxdb");

		while (const SRC::RecordData *pRec = file.ReadRecord())
		{
			for (unsigned x=0; x<file.m_recordInfo.NumFields(); ++x)
			{
				const SRC::FieldBase * pField = file.m_recordInfo[x];
				if (pField->m_ft!=SRC::E_FT_SpatialObj)
				{
					if (x!=0)
						std::cout << ",";
					std::cout << pField->GetAsAString(pRec).value.pValue;
				}
			}
			std::cout << "\n";
		}
	}
	catch (SRC::Error e)
	{
		std::cout << SRC::ConvertToAString(e.GetErrorDescription()) << "\n";
	}
	return 0;
}

