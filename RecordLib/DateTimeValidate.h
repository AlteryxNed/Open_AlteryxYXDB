#pragma once


namespace SRC
{
	template <typename TChar> class TDateTimeValidate
	{
		inline static bool IsDigit(TChar c)
		{
			return c >= '0' && c <= '9';
		}
		inline static int ToInt(const TChar *p);
	public:
		static bool ValidateDate(const TChar *pVal, int nLen);
		static bool ValidateTime(const TChar *pVal, int nLen);
		static bool ValidateDateTime(const TChar *pVal, int nLen);
	};
	template <> inline int TDateTimeValidate<char>::ToInt(const char *p)
	{
		return atol(p);
	}
	template <> inline int TDateTimeValidate<wchar_t>::ToInt(const wchar_t *p)
	{
		return _wtol(p);
	}
	template <typename TChar> bool TDateTimeValidate<TChar>::ValidateDate(const TChar *pVal, int nLen)
	{
		if (nLen == 10
			&& IsDigit(pVal[0])
			&& IsDigit(pVal[1])
			&& IsDigit(pVal[2])
			&& IsDigit(pVal[3])
			&& pVal[4] == '-'
			&& IsDigit(pVal[5])
			&& IsDigit(pVal[6])
			&& pVal[7] == '-'
			&& IsDigit(pVal[8])
			&& IsDigit(pVal[9]))
		{
			unsigned short nMonth = static_cast<unsigned short>(ToInt(pVal + 5));
			unsigned short nDay = static_cast<unsigned short>(ToInt(pVal + 8));
			unsigned nYear = ToInt(pVal);
			if (nYear < 1400)
				return false;
			switch (nMonth)
			{
			case 1:
			case 3:
			case 5:
			case 7:
			case 8:
			case 10:
			case 12:
				if (nDay > 31)
					return false;
				break;
			case 2:
			{
				if (nYear % 4 == 0)
				{
					if (nDay > 29 ||
						(nDay == 29 && nYear % 100 == 0 && nYear % 400 != 0))
						return false;
				}
				else if (nDay > 28)
					return false;
			}
				break;
			case 4:
			case 6:
			case 9:
			case 11:
				if (nDay > 30)
					return false;
				break;
			default:
				return false;
			}
			if (nDay == 0)
				return false;

			return true;
		}

		return false;
	}



	template <typename TChar> bool TDateTimeValidate<TChar>::ValidateTime(const TChar *pVal, int nLen)
	{
		if (nLen == 8
			&& IsDigit(pVal[0])
			&& IsDigit(pVal[1])
			&& pVal[2] == ':'
			&& IsDigit(pVal[3])
			&& IsDigit(pVal[4])
			&& pVal[5] == ':'
			&& IsDigit(pVal[6])
			&& IsDigit(pVal[7]))
		{
			if (static_cast<unsigned short>(ToInt(pVal)) >= 24)
				return false;
			if (static_cast<unsigned short>(ToInt(pVal + 3)) >= 60)
				return false;
			if (static_cast<unsigned short>(ToInt(pVal + 6)) >= 60)
				return false;

			return true;
		}
		return false;
	}
	template <typename TChar> bool TDateTimeValidate<TChar>::ValidateDateTime(const TChar *pVal, int nLen)
	{
		if (nLen == 10)
			return ValidateDate(pVal, 10);
		else if (nLen == 19)
			return ValidateDate(pVal, 10) && pVal[10] == ' ' && ValidateTime(pVal + 11, 8);
		else
			return false;

	}
}
