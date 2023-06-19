#include "muParserInt.h"

#include <cmath>
#include <algorithm>
#include <numeric>

using namespace std;

/** \file
	\brief Implementation of a parser using integer value.
*/

/** \brief 数学应用的命名空间。 */
namespace mu
{
	value_type ParserInt::Abs(value_type v) { return (value_type)Round(fabs((double)v)); }  // 绝对值函数
	value_type ParserInt::Sign(value_type v) { return (Round(v) < 0) ? -1 : (Round(v) > 0) ? 1 : 0; }  // 符号函数
	value_type ParserInt::Ite(value_type v1, value_type v2, value_type v3) { return (Round(v1) == 1) ? Round(v2) : Round(v3); }  // 条件选择函数
	value_type ParserInt::Add(value_type v1, value_type v2) { return Round(v1) + Round(v2); }  // 加法函数
	value_type ParserInt::Sub(value_type v1, value_type v2) { return Round(v1) - Round(v2); }  // 减法函数
	value_type ParserInt::Mul(value_type v1, value_type v2) { return Round(v1) * Round(v2); }  // 乘法函数
	value_type ParserInt::Div(value_type v1, value_type v2) { return Round(v1) / Round(v2); }  // 除法函数
	value_type ParserInt::Mod(value_type v1, value_type v2) { return Round(v1) % Round(v2); }  // 取模函数
	value_type ParserInt::Shr(value_type v1, value_type v2) { return Round(v1) >> Round(v2); }  // 右移函数
	value_type ParserInt::Shl(value_type v1, value_type v2) { return Round(v1) << Round(v2); }  // 左移函数
	value_type ParserInt::BitAnd(value_type v1, value_type v2) { return Round(v1) & Round(v2); }  // 位与函数
	value_type ParserInt::BitOr(value_type v1, value_type v2) { return Round(v1) | Round(v2); }  // 位或函数
	value_type ParserInt::And(value_type v1, value_type v2) { return Round(v1) && Round(v2); }  // 逻辑与函数
	value_type ParserInt::Or(value_type v1, value_type v2) { return Round(v1) || Round(v2); }  // 逻辑或函数
	value_type ParserInt::Less(value_type v1, value_type v2) { return Round(v1) < Round(v2); }  // 小于函数
	value_type ParserInt::Greater(value_type v1, value_type v2) { return Round(v1) > Round(v2); }  // 大于函数
	value_type ParserInt::LessEq(value_type v1, value_type v2) { return Round(v1) <= Round(v2); }  // 小于等于函数
	value_type ParserInt::GreaterEq(value_type v1, value_type v2) { return Round(v1) >= Round(v2); }  // 大于等于函数
	value_type ParserInt::Equal(value_type v1, value_type v2) { return Round(v1) == Round(v2); }  // 等于函数
	value_type ParserInt::NotEqual(value_type v1, value_type v2) { return Round(v1) != Round(v2); }  // 不等于函数
	value_type ParserInt::Not(value_type v) { return !Round(v); }  // 逻辑非函数

	value_type ParserInt::Pow(value_type v1, value_type v2)
	{
		return std::pow((double)Round(v1), (double)Round(v2));  // 幂函数
	}


	value_type ParserInt::UnaryMinus(value_type v)
	{
		return -Round(v);  // 一元减运算符
	}


	value_type ParserInt::Sum(const value_type* a_afArg, int a_iArgc)
	{
		if (!a_iArgc)
			throw ParserError(_T("too few arguments for function sum."));

		value_type fRes = 0;
		for (int i = 0; i < a_iArgc; ++i)
			fRes += a_afArg[i];

		return fRes;  // 求和函数
	}


	value_type ParserInt::Min(const value_type* a_afArg, int a_iArgc)
	{
		if (!a_iArgc)
			throw ParserError(_T("too few arguments for function min."));

		value_type fRes = a_afArg[0];
		for (int i = 0; i < a_iArgc; ++i)
			fRes = std::min(fRes, a_afArg[i]);

		return fRes;  // 最小值函数
	}


	value_type ParserInt::Max(const value_type* a_afArg, int a_iArgc)
	{
		if (!a_iArgc)
			throw ParserError(_T("too few arguments for function min."));

		value_type fRes = a_afArg[0];
		for (int i = 0; i < a_iArgc; ++i)
			fRes = std::max(fRes, a_afArg[i]);

		return fRes;  // 最大值函数
	}


	int ParserInt::IsVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		string_type buf(a_szExpr);
		std::size_t pos = buf.find_first_not_of(_T("0123456789"));

		if (pos == std::string::npos)
			return 0;

		stringstream_type stream(buf.substr(0, pos));
		int iVal(0);

		stream >> iVal;
		if (stream.fail())
			return 0;

		stringstream_type::pos_type iEnd = stream.tellg();   // 读取后的位置
		if (stream.fail())
			iEnd = stream.str().length();

		if (iEnd == (stringstream_type::pos_type) - 1)
			return 0;

		*a_iPos += (int)iEnd;
		*a_fVal = (value_type)iVal;
		return 1;
	}


	/** \brief 检查表达式中的某个位置是否存在十六进制值。
		\param a_szExpr 指向表达式字符串的指针
		\param [in/out] a_iPos 指向整数值，保存当前解析位置
		\param [out] a_fVal 指向将被存储检测到的值的位置。

		十六进制值必须以 "0x" 作为前缀才能正确检测。
	*/
	int ParserInt::IsHexVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		if (a_szExpr[1] == 0 || (a_szExpr[0] != '0' || a_szExpr[1] != 'x'))
			return 0;

		unsigned iVal(0);

		// 基于流的新代码，以支持UNICODE:
		stringstream_type::pos_type nPos(0);
		stringstream_type ss(a_szExpr + 2);
		ss >> std::hex >> iVal;
		nPos = ss.tellg();

		if (nPos == (stringstream_type::pos_type)0)
			return 1;

		*a_iPos += (int)(2 + nPos);
		*a_fVal = (value_type)iVal;
		return 1;
	}


	int ParserInt::IsBinVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		if (a_szExpr[0] != '#')
			return 0;

		unsigned iVal(0),
			iBits(sizeof(iVal) * 8),
			i(0);

		for (i = 0; (a_szExpr[i + 1] == '0' || a_szExpr[i + 1] == '1') && i < iBits; ++i)
			iVal |= (int)(a_szExpr[i + 1] == '1') << ((iBits - 1) - i);

		if (i == 0)
			return 0;

		if (i == iBits)
			throw exception_type(_T("Binary to integer conversion error (overflow)."));

		*a_fVal = (unsigned)(iVal >> (iBits - i));
		*a_iPos += i + 1;

		return 1;
	}


	/** \brief 构造函数。

		调用 ParserBase 类的构造函数，并触发 Function、Operator 和 Constant 的初始化。
	*/
	ParserInt::ParserInt()
		:ParserBase()
	{
		AddValIdent(IsVal);    // 优先级最低
		AddValIdent(IsBinVal);
		AddValIdent(IsHexVal); // 优先级最高

		InitCharSets();
		InitFun();
		InitOprt();
	}


	void ParserInt::InitConst()
	{
	}


	void ParserInt::InitCharSets()
	{
		DefineNameChars(_T("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		DefineOprtChars(_T("+-*^/?<>=!%&|~'_"));
		DefineInfixOprtChars(_T("/+-*^?<>=!%&|~'_"));
	}


	/** \brief 初始化默认函数。 */
	void ParserInt::InitFun()
	{
		DefineFun(_T("sign"), Sign);
		DefineFun(_T("abs"), Abs);
		DefineFun(_T("if"), Ite);
		DefineFun(_T("sum"), Sum);
		DefineFun(_T("min"), Min);
		DefineFun(_T("max"), Max);
	}


	/** \brief 初始化操作符。 */
	void ParserInt::InitOprt()
	{
		// 禁用所有内置操作符，不都适用于整数数字（它们不会进行四舍五入）
		// (they don't do rounding of values)
		EnableBuiltInOprt(false);

		// 禁用逻辑操作符
		// 因为它们设计用于浮点数
		DefineInfixOprt(_T("-"), UnaryMinus);
		DefineInfixOprt(_T("!"), Not);

		DefineOprt(_T("&"), BitAnd, prBAND);
		DefineOprt(_T("|"), BitOr, prBOR);

		DefineOprt(_T("&&"), And, prLAND);
		DefineOprt(_T("||"), Or, prLOR);

		DefineOprt(_T("<"), Less, prCMP);
		DefineOprt(_T(">"), Greater, prCMP);
		DefineOprt(_T("<="), LessEq, prCMP);
		DefineOprt(_T(">="), GreaterEq, prCMP);
		DefineOprt(_T("=="), Equal, prCMP);
		DefineOprt(_T("!="), NotEqual, prCMP);

		DefineOprt(_T("+"), Add, prADD_SUB);
		DefineOprt(_T("-"), Sub, prADD_SUB);

		DefineOprt(_T("*"), Mul, prMUL_DIV);
		DefineOprt(_T("/"), Div, prMUL_DIV);
		DefineOprt(_T("%"), Mod, prMUL_DIV);

		DefineOprt(_T("^"), Pow, prPOW, oaRIGHT);
		DefineOprt(_T(">>"), Shr, prMUL_DIV + 1);
		DefineOprt(_T("<<"), Shl, prMUL_DIV + 1);
	}

} // namespace mu
