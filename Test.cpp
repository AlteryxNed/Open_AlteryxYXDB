
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
void WriteSampleFile(const wchar_t *pFile)
{
	// the RecordInfo structure defines a record for the YXDB file.
	SRC::RecordInfo recordInfoOut;

	// Before you can create a YXDB, you need to tell the RecordInfo what fields it will expect
	// Use CreateFieldXML to properly format the XML that describes a field
	recordInfoOut.AddField(SRC::RecordInfo::CreateFieldXml(L"Number", SRC::E_FT_Double));
	recordInfoOut.AddField(SRC::RecordInfo::CreateFieldXml(L"English", SRC::E_FT_V_String, 256));

	//Now that we have defined the fields, we can create the file.
	Alteryx::OpenYXDB::Open_AlteryxYXDB fileOut;

	fileOut.Create(pFile, recordInfoOut.GetRecordXmlMetaData());

	// in order to add a record to the file, we need to create an empty record and then fill it in
	SRC::SmartPointerRefObj<SRC::Record> pRec = recordInfoOut.CreateRecord();

	// just creating some random #'s to have some data to put into the file.
	std::mt19937 r;
	for (unsigned x = 0; x<100; ++x)
	{
		// this is very important.  ALWAYS reset the record to clear out all the variable length data
		// if you skip this step, it will appear to work, but the record will get larger and larger
		// generating an extremely inefficient YXDB - and eventually running out of memory.
		// this will not actually clear out the values though.  You still need to either call:
		//		SetFromXXX(...)
		//	or
		//		SetNull(...)
		// for every field in the record.
		pRec->Reset();
		int v = r();

		// the recordInfo object contains an array for FieldBase objects.
		// although the fields are strongly typed, the field object acts like a variant - 
		// which is to say it will accept setting the field as a different type than what it is writing

		// in this case we are setting the value as an integer, but the field is actually a float.
		// that is OK because the Field library will convert it for us.
		recordInfoOut[0]->SetFromInt32(pRec.Get(), v);
		recordInfoOut[1]->SetFromString(pRec.Get(), EnglishNumber(v));

		// now that we have filled out the record, we can add it to the file
		fileOut.AppendRecord(pRec->GetRecord());
	}

	// calling Close is actually optional, since the destructor will call it for you.
	// but if you don't explicitly call it, it will not be able to throw exceptions if the final 
	// write fails for any reason
	fileOut.Close();
}

void ReadSampleFile(const wchar_t *pFile)
{
	Alteryx::OpenYXDB::Open_AlteryxYXDB file;
	file.Open(pFile);

	// you can ask about how many fields are in the file, what are there names and types, etc...
	for (unsigned x = 0; x < file.m_recordInfo.NumFields(); ++x)
	{
		if (x != 0)
			std::cout << ",";

		// the FieldBase object has all kinds of information about the field
		// it will also help us (later) get a specific value from a record
		const SRC::FieldBase * pField = file.m_recordInfo[x];
		std::cout << SRC::ConvertToAString(pField->GetFieldName().c_str());
	}
	std::cout << "\n";

	// read 1 record at a time from the YXDB.  When the file as read past
	// the last record, ReadRecord will return nullptr
	// You could have also called file.GetNumRecords() to know the total ahead of time
	while (const SRC::RecordData *pRec = file.ReadRecord())
	{
		// we now have a record (pRec) but it is an opaque structure
		// we need to use the FieldBase objects to get actual values from it.
		for (unsigned x = 0; x<file.m_recordInfo.NumFields(); ++x)
		{
			// the recordInfo object acts like an array of FieldBase objects
			const SRC::FieldBase * pField = file.m_recordInfo[x];

			// binary fields are not implicitly convertable to strings
			if (!IsBinary(pField->m_ft))
			{
				if (x != 0)
					std::cout << ",";

				// you could (and probably should) as for GetAsWString to get the unicode value
				std::cout << pField->GetAsAString(pRec).value.pValue;
			}
		}
		std::cout << "\n";
	}
}
int _tmain(int argc, _TCHAR* argv[])
{
	// most of the functions in this library can throw class Error if something goes wrong
	try
	{
		WriteSampleFile(L"temp.yxdb");
		ReadSampleFile(L"temp.yxdb");
	}
	catch (SRC::Error e)
	{
		std::cout << SRC::ConvertToAString(e.GetErrorDescription()) << "\n";
	}
	return 0;
}

