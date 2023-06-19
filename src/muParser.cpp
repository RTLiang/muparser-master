#include "muParser.h"
#include "muParserTemplateMagic.h"

//--- Standard includes ------------------------------------------------------------------------
#include <cmath>
#include <algorithm>
#include <numeric>

using namespace std;

/** \file
	\brief Implementation of the standard floating point parser.
*/


/** \brief 数学应用的命名空间。 */
namespace mu
{
	//---------------------------------------------------------------------------
	/** \brief 默认的值识别回调函数。
		\param [in] a_szExpr 表达式指针
		\param [in, out] a_iPos 存储当前位置的索引指针
		\param [out] a_fVal 在找到值时应存储值的指针。
		\return 如果找到值返回1，否则返回0。
	*/
	int Parser::IsVal(const char_type* a_szExpr, int* a_iPos, value_type* a_fVal)
	{
		value_type fVal(0);

		stringstream_type stream(a_szExpr);
		stream.seekg(0);        // todo:  check if this really is necessary
		stream.imbue(Parser::s_locale);
		stream >> fVal;
		stringstream_type::pos_type iEnd = stream.tellg(); // 读取后的位置

		if (iEnd == (stringstream_type::pos_type) - 1)
			return 0;

		*a_iPos += (int)iEnd;
		*a_fVal = fVal;
		return 1;
	}


	//---------------------------------------------------------------------------
	/** \brief 构造函数。

	  调用ParserBase类的构造函数并触发函数、操作符和常量的初始化。
	*/
	Parser::Parser()
		:ParserBase()
	{
		AddValIdent(IsVal);

		InitCharSets();
		InitFun();
		InitConst();
		InitOprt();
	}

	//---------------------------------------------------------------------------
	/** \brief 定义字符集。
		\sa DefineNameChars, DefineOprtChars, DefineInfixOprtChars

	  该函数用于初始化默认字符集，定义在函数和变量名以及操作符中可用的字符。
	*/
	void Parser::InitCharSets()
	{
		DefineNameChars(_T("0123456789_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"));
		DefineOprtChars(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-*^/?<>=#!$%&|~'_{}"));
		DefineInfixOprtChars(_T("/+-*^?<>=#!$%&|~'_"));
	}

	//---------------------------------------------------------------------------
	/** \brief 初始化默认函数。 */
	void Parser::InitFun()
	{
		if (mu::TypeInfo<mu::value_type>::IsInteger())
		{
			// 当将MUP_BASETYPE设置为整数类型时
			// 在此处放置处理整数值的函数
			// ...
			// ...
			// ...
		}
		else
		{
			// 三角函数
			DefineFun(_T("sin"), MathImpl<value_type>::Sin);
			DefineFun(_T("cos"), MathImpl<value_type>::Cos);
			DefineFun(_T("tan"), MathImpl<value_type>::Tan);
			// 反三角函数
			DefineFun(_T("asin"), MathImpl<value_type>::ASin);
			DefineFun(_T("acos"), MathImpl<value_type>::ACos);
			DefineFun(_T("atan"), MathImpl<value_type>::ATan);
			DefineFun(_T("atan2"), MathImpl<value_type>::ATan2);
			// 双曲线函数
			DefineFun(_T("sinh"), MathImpl<value_type>::Sinh);
			DefineFun(_T("cosh"), MathImpl<value_type>::Cosh);
			DefineFun(_T("tanh"), MathImpl<value_type>::Tanh);
			// 反双曲线函数
			DefineFun(_T("asinh"), MathImpl<value_type>::ASinh);
			DefineFun(_T("acosh"), MathImpl<value_type>::ACosh);
			DefineFun(_T("atanh"), MathImpl<value_type>::ATanh);
			// 对数函数
			DefineFun(_T("log2"), MathImpl<value_type>::Log2);
			DefineFun(_T("log10"), MathImpl<value_type>::Log10);
			DefineFun(_T("log"), MathImpl<value_type>::Log);
			DefineFun(_T("ln"), MathImpl<value_type>::Log);
			// 其他
			DefineFun(_T("exp"), MathImpl<value_type>::Exp);
			DefineFun(_T("sqrt"), MathImpl<value_type>::Sqrt);
			DefineFun(_T("sign"), MathImpl<value_type>::Sign);
			DefineFun(_T("rint"), MathImpl<value_type>::Rint);
			DefineFun(_T("abs"), MathImpl<value_type>::Abs);
			// 带有可变参数的函数
			DefineFun(_T("sum"), MathImpl<value_type>::Sum);
			DefineFun(_T("avg"), MathImpl<value_type>::Avg);
			DefineFun(_T("min"), MathImpl<value_type>::Min);
			DefineFun(_T("max"), MathImpl<value_type>::Max);
		}
	}

	//---------------------------------------------------------------------------
	/** \brief 初始化常量。

	  默认情况下，解析器识别两个常量：圆周率（"pi"）和自然对数的底数（"_e"）。
	*/
	void Parser::InitConst()
	{
		DefineConst(_T("_pi"), MathImpl<value_type>::CONST_PI);
		DefineConst(_T("_e"), MathImpl<value_type>::CONST_E);
	}

	//---------------------------------------------------------------------------
	/** \brief 初始化操作符。

	  默认情况下，只添加了一元减号操作符。
	*/
	void Parser::InitOprt()
	{
		DefineInfixOprt(_T("-"), MathImpl<value_type>::UnaryMinus);
		DefineInfixOprt(_T("+"), MathImpl<value_type>::UnaryPlus);
	}

	//---------------------------------------------------------------------------
	void Parser::OnDetectVar(string_type* /*pExpr*/, int& /*nStart*/, int& /*nEnd*/)
	{
		// 这只是演示代码，用于说明动态修改变量名的功能。
		// 我不确定是否有人真正需要这样的功能...
		/*


		string sVar(pExpr->begin()+nStart, pExpr->begin()+nEnd);
		string sRepl = std::string("_") + sVar + "_";

		int nOrigVarEnd = nEnd;
		cout << "variable detected!\n";
		cout << "  Expr: " << *pExpr << "\n";
		cout << "  Start: " << nStart << "\n";
		cout << "  End: " << nEnd << "\n";
		cout << "  Var: \"" << sVar << "\"\n";
		cout << "  Repl: \"" << sRepl << "\"\n";
		nEnd = nStart + sRepl.length();
		cout << "  End: " << nEnd << "\n";
		pExpr->replace(pExpr->begin()+nStart, pExpr->begin()+nOrigVarEnd, sRepl);
		cout << "  New expr: " << *pExpr << "\n";
		*/
	}

	//---------------------------------------------------------------------------
	/** \brief 数值微分。

		\param [in] a_Var 指向微分变量的指针。
		\param [in] a_fPos 执行微分的位置。
		\param [in] a_fEpsilon 数值微分使用的 epsilon 值。

		数值微分使用 5 点算子计算，得到 4 阶公式。epsilon 的默认值为 0.00074，
		即 numeric_limits<double>::epsilon() ^ (1/5)。
	*/
	value_type Parser::Diff(value_type* a_Var, value_type  a_fPos, value_type  a_fEpsilon) const
	{
		value_type fRes(0);
		value_type fBuf(*a_Var);
		value_type f[4] = { 0,0,0,0 };
		value_type fEpsilon(a_fEpsilon);

		// 为了向后兼容，如果用户没有提供自己的 epsilon，则计算 epsilon 的值
		if (fEpsilon == 0)
			fEpsilon = (a_fPos == 0) ? (value_type)1e-10 : (value_type)1e-7 * a_fPos;

		*a_Var = a_fPos + 2 * fEpsilon;  f[0] = Eval();
		*a_Var = a_fPos + 1 * fEpsilon;  f[1] = Eval();
		*a_Var = a_fPos - 1 * fEpsilon;  f[2] = Eval();
		*a_Var = a_fPos - 2 * fEpsilon;  f[3] = Eval();
		*a_Var = fBuf; // 恢复变量

		fRes = (-f[0] + 8 * f[1] - 8 * f[2] + f[3]) / (12 * fEpsilon);
		return fRes;
	}
} // namespace mu

//程序实现了一个用于解析数学表达式的浮点数解析器，并提供了函数、运算符和常量的初始化功能。解析器可以识别各种数学函数和运算符，包括三角函数、指数函数、对数函数等。此外，解析器还提供了微分功能，可以对表达式进行数值微分操作。
