#include <windows.h>
#include "intrinsic.h"
#define USING_NAMESPACE_BCB6_STD
#include "TStringDivision.h"

#define ESCAPE_TAG          '\\'
#define MAX_NEST_TAG_LENGTH 2

unsigned long __cdecl TStringDivision_FindBack(
	IN     TStringDivision *this,
	IN     const string    *Src,
	IN     string          Token,
	IN     unsigned long   FromIndex,
	IN     unsigned long   ToIndex,
	IN     unsigned long   Option)
{
	size_t TokenLength;
	size_t SrcLength;
	LPCSTR SrcIt, SrcEnd;
	size_t NestStartTagLength;
	size_t NestEndTagLength;
	size_t FindIndex;

	if (ToIndex == FromIndex)
		goto FAILED;

	TokenLength = string_length(&Token);
	SrcLength = string_length(Src);

	// 原文より比較文の方が短いなんて論外(^^;
	if (SrcLength < TokenLength)
		goto FAILED;

	// 検索終端がおかしければ補正
	if (SrcLength < FromIndex || SrcLength < FromIndex + TokenLength)
		FromIndex = SrcLength - TokenLength + 1;

	SrcIt = string_c_str(Src) + ToIndex;
	SrcEnd = string_c_str(Src) + FromIndex;

	if (Option & dtNEST)
	{
		NestStartTagLength = string_length(&this->nestStartTag);
		NestEndTagLength = string_length(&this->nestEndTag);
		if (NestStartTagLength == 0 || NestEndTagLength == 0)
			goto FAILED;
	}

	// ただのパターンマッチングならBoyer-Moore法って手もあるが、
	// 2バイト文字やネスト、エスケープシーケンスも許可しているので1つづつ(^^;)

	FindIndex = (unsigned long)SIZE_MAX;
	if (Option & dtNEST)
	{
		// ネストチェックあり
		if (Option & dtESCAPE)
		{
			// エスケープシーケンス使用
			while (SrcIt < SrcEnd)
			{
				if (*SrcIt != ESCAPE_TAG)
				{
					if (SrcIt[0] == string_at(&this->nestStartTag, 0) && (NestStartTagLength <= 1 || SrcIt[1] == string_at(&this->nestStartTag, 1)))
					{
						size_t NCount;

						// ネスト開始
						NCount = 1;
						SrcIt += NestStartTagLength;
						while (SrcIt < SrcEnd)
						{
							if (*SrcIt != ESCAPE_TAG)
							{
								if (SrcIt[0] == string_at(&this->nestStartTag, 0) && (NestStartTagLength <= 1 || SrcIt[1] == string_at(&this->nestStartTag, 1)))
								{
									// さらにネスト
									SrcIt += NestStartTagLength;
									NCount++;
									continue;
								}
								if (SrcIt[0] == string_at(&this->nestEndTag, 0) && (NestEndTagLength <= 1 || SrcIt[1] == string_at(&this->nestEndTag, 1)))
								{
									// ネスト(一段)解除
									SrcIt += NestEndTagLength;
									if (--NCount == 0)
										break;	// ネスト完全脱出
									continue;
								}
							}
							else
							{
								SrcIt++;
							}
							if (!__intrinsic_isleadbyte(*SrcIt))
								SrcIt++;
							else
								SrcIt += 2;
						}
						continue;	// 直後にまたネスト開始もありえるでの。
					}

					// 基本比較処理
					if (memcmp(SrcIt, string_c_str(&Token), TokenLength) == 0)
						FindIndex = SrcIt - string_c_str(Src);
				}
				else
				{
					// エスケープシーケンスに引っかかりました
					SrcIt++;
				}

				if (!__intrinsic_isleadbyte(*SrcIt))
					SrcIt++;
				else
					SrcIt += 2;
			}
		}
		else
		{
			LPCSTR const SrcLow = SrcIt;
			SrcIt = SrcEnd;
			while (--SrcIt >= SrcLow)
			{
				if (SrcIt > SrcLow && __intrinsic_isleadbyte(SrcIt[-1]))
					SrcIt--;
				if (SrcIt[0] == string_at(&this->nestEndTag, 0) && (NestEndTagLength <= 1 || SrcIt[1] == string_at(&this->nestEndTag, 1)))
				{
					size_t NCount;

					// ネスト開始
					NCount = 1;
					while (--SrcIt >= SrcLow)
					{
						if (SrcIt > SrcLow && __intrinsic_isleadbyte(SrcIt[-1]))
							SrcIt--;
						if (SrcIt[0] == string_at(&this->nestEndTag, 0) && (NestEndTagLength <= 1 || SrcIt[1] == string_at(&this->nestEndTag, 1)))
						{
							// さらにネスト
							NCount++;
							continue;
						}
						if (SrcIt[0] == string_at(&this->nestStartTag, 0) && (NestStartTagLength <= 1 || SrcIt[1] == string_at(&this->nestStartTag, 1)))
						{
							// ネスト(一段)解除
							if (--NCount == 0)
								break;	// ネスト完全脱出
							continue;
						}
					}
					continue;	// 直後にまたネスト開始もありえるでの。
				}

				// 基本比較処理
				if (SrcEnd - SrcIt < (ptrdiff_t)TokenLength)
					continue;
				if (memcmp(SrcIt, string_c_str(&Token), TokenLength) == 0)
				{
					FindIndex = SrcIt - string_c_str(Src);
					break;
				}
			}
		}
	}
	else if (Option & dtESCAPE)
	{
		// エスケープシーケンス使用
		while (SrcIt < SrcEnd)
		{
			if (*SrcIt != ESCAPE_TAG)
			{
				// 基本比較処理
				if (memcmp(SrcIt, string_c_str(&Token), TokenLength) == 0)
					FindIndex = SrcIt - string_c_str(Src);
			}
			else
			{
				// エスケープシーケンスに引っかかりました
				SrcIt++;
			}
			if (!__intrinsic_isleadbyte(*SrcIt))
				SrcIt++;
			else
				SrcIt += 2;
		}
	}
	else
	{
		// 単純検索
		while (SrcIt < SrcEnd)
		{
			// 基本比較処理
			if (memcmp(SrcIt, string_c_str(&Token), TokenLength) == 0)
				FindIndex = SrcIt - string_c_str(Src);

			if (!__intrinsic_isleadbyte(*SrcIt))
				SrcIt++;
			else
				SrcIt += 2;
		}
	}

	string_dtor(&Token);
	return (unsigned long)FindIndex;

FAILED:
	string_dtor(&Token);
	return (unsigned long)SIZE_MAX;
}

