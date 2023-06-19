/*

	 _____  __ _____________ _______  ______ ___________
	/     \|  |  \____ \__  \\_  __ \/  ___// __ \_  __ \
   |  Y Y  \  |  /  |_> > __ \|  | \/\___ \\  ___/|  | \/
   |__|_|  /____/|   __(____  /__|  /____  >\___  >__|
		 \/      |__|       \/           \/     \/
   Copyright (C) 2022 Ingo Berg

	Redistribution and use in source and binary forms, with or without modification, are permitted
	provided that the following conditions are met:

	  * Redistributions of source code must retain the above copyright notice, this list of
		conditions and the following disclaimer.
	  * Redistributions in binary form must reproduce the above copyright notice, this list of
		conditions and the following disclaimer in the documentation and/or other materials provided
		with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
	IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
	OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "muParserBase.h"
#include "muParserTemplateMagic.h"

//--- Standard includes ------------------------------------------------------------------------
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <deque>
#include <sstream>
#include <locale>
#include <cassert>
#include <cctype>

#ifdef MUP_USE_OPENMP

#include <omp.h>

#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26812)
#endif

using namespace std;

/** \file
	\brief This file contains the basic implementation of the muparser engine.
*/
// 这是一个C++代码文件，实现了muparser引擎的基本功能。

// 代码实现了muparser引擎的基本功能。muparser是一个用于解析和计算数学表达式的C++库，它可以处理各种数学函数和运算符，并提供了一个灵活的表达式解析器。这段代码包含了muparser引擎的基本实现，包括必要的头文件和命名空间声明。代码中还包含了一些标准库的引用和条件编译指令，用于特定平台和编译器的兼容性处理。
namespace mu
{
	std::locale ParserBase::s_locale = std::locale(std::locale::classic(), new change_dec_sep<char_type>('.'));

	bool ParserBase::g_DbgDumpCmdCode = false;
	bool ParserBase::g_DbgDumpStack = false;

	//------------------------------------------------------------------------------
	/** \brief 内置二元运算符的标识符。

		在使用 #AddOprt(...) 定义自定义二元运算符时，确保不选择与这些定义冲突的名称。
	*/
	const char_type *ParserBase::c_DefaultOprt[] =
		{
			_T("<="), _T(">="), _T("!="),
			_T("=="), _T("<"), _T(">"),
			_T("+"), _T("-"), _T("*"),
			_T("/"), _T("^"), _T("&&"),
			_T("||"), _T("="), _T("("),
			_T(")"), _T("?"), _T(":"), 0};

	const int ParserBase::s_MaxNumOpenMPThreads = 16;

	//------------------------------------------------------------------------------
	/** \brief 构造函数。
		\param a_szFormula 要解释的公式。
		\throw ParserException 如果 a_szFormula 为 nullptr。
	*/
	ParserBase::ParserBase()
		: m_pParseFormula(&ParserBase::ParseString), m_vRPN(), m_vStringBuf(), m_pTokenReader(), m_FunDef(), m_PostOprtDef(), m_InfixOprtDef(), m_OprtDef(), m_ConstDef(), m_StrVarDef(), m_VarDef(), m_bBuiltInOp(true), m_sNameChars(), m_sOprtChars(), m_sInfixOprtChars(), m_vStackBuffer(), m_nFinalResultIdx(0)
	{
		InitTokenReader();
	}

	//---------------------------------------------------------------------------
	/** \brief 拷贝构造函数。

	  解析器可以被安全地拷贝构造，但字节码在拷贝构造过程中被重置。
	*/
	ParserBase::ParserBase(const ParserBase &a_Parser)
		: m_pParseFormula(&ParserBase::ParseString), m_vRPN(), m_vStringBuf(), m_pTokenReader(), m_FunDef(), m_PostOprtDef(), m_InfixOprtDef(), m_OprtDef(), m_ConstDef(), m_StrVarDef(), m_VarDef(), m_bBuiltInOp(true), m_sNameChars(), m_sOprtChars(), m_sInfixOprtChars()
	{
		m_pTokenReader.reset(new token_reader_type(this));
		Assign(a_Parser);
	}

	//---------------------------------------------------------------------------
	ParserBase::~ParserBase()
	{
	}

	//---------------------------------------------------------------------------
	/** \brief 赋值运算符。

	  通过调用 Assign(a_Parser) 实现。抑制了自我赋值。
	  \param a_Parser 要复制到此处的对象。
	  \return *this
	  \throw nothrow
	*/
	ParserBase &ParserBase::operator=(const ParserBase &a_Parser)
	{
		Assign(a_Parser);
		return *this;
	}

	//---------------------------------------------------------------------------
	/** \brief 将解析器对象的状态复制到此处。

	  清除此解析器的变量和函数。
	  复制所有内部变量的状态。
	  重置解析函数为字符串解析模式。

	  \param a_Parser 源对象。
	*/
	void ParserBase::Assign(const ParserBase &a_Parser)
	{
		if (&a_Parser == this)
			return;

		// 不复制字节码，而是通过重置解析函数来导致解析器创建新的字节码。
		ReInit();

		m_ConstDef = a_Parser.m_ConstDef; // 复制用户定义的常量
		m_VarDef = a_Parser.m_VarDef;	  // 复制用户定义的变量
		m_bBuiltInOp = a_Parser.m_bBuiltInOp;
		m_vStringBuf = a_Parser.m_vStringBuf;
		m_vStackBuffer = a_Parser.m_vStackBuffer;
		m_nFinalResultIdx = a_Parser.m_nFinalResultIdx;
		m_StrVarDef = a_Parser.m_StrVarDef;
		m_vStringVarBuf = a_Parser.m_vStringVarBuf;
		m_pTokenReader.reset(a_Parser.m_pTokenReader->Clone(this));

		// 复制函数和运算符回调
		m_FunDef = a_Parser.m_FunDef;			  // 复制函数定义
		m_PostOprtDef = a_Parser.m_PostOprtDef;	  // 后值一元运算符
		m_InfixOprtDef = a_Parser.m_InfixOprtDef; // 中缀表示法的一元运算符
		m_OprtDef = a_Parser.m_OprtDef;			  // 二元运算符

		m_sNameChars = a_Parser.m_sNameChars;
		m_sOprtChars = a_Parser.m_sOprtChars;
		m_sInfixOprtChars = a_Parser.m_sInfixOprtChars;
	}
	// 这段代码是一个C++解析器的实现，它提供了对数学表达式的解析和求值功能。其中包含了以下功能：

	// 初始化解析器的构造函数。
	// 拷贝构造函数和赋值运算符，用于创建解析器对象的副本。
	// 将解析器对象的状态复制到当前对象的函数。
	// 定义了一些内置的二元运算符。
	// 存储解析器的状态和数据的成员变量。

	//---------------------------------------------------------------------------
	/** \brief 设置小数分隔符。
		\param cDecSep 小数分隔符的字符值。
		\sa SetThousandsSep

		默认情况下，muparser使用"C"区域设置。此区域的小数分隔符会被此处提供的分隔符覆盖。
	*/
	void ParserBase::SetDecSep(char_type cDecSep)
	{
		char_type cThousandsSep = std::use_facet<change_dec_sep<char_type>>(s_locale).thousands_sep();
		s_locale = std::locale(std::locale("C"), new change_dec_sep<char_type>(cDecSep, cThousandsSep));
	}

	//---------------------------------------------------------------------------
	/** \brief 设置千位分隔符。
		\param cThousandsSep 千位分隔符的字符。
		\sa SetDecSep

		默认情况下，muparser使用"C"区域设置。此区域的千位分隔符会被此处提供的分隔符覆盖。
	*/
	void ParserBase::SetThousandsSep(char_type cThousandsSep)
	{
		char_type cDecSep = std::use_facet<change_dec_sep<char_type>>(s_locale).decimal_point();
		s_locale = std::locale(std::locale("C"), new change_dec_sep<char_type>(cDecSep, cThousandsSep));
	}

	//---------------------------------------------------------------------------
	/** \brief 重置区域设置。

	  默认区域设置使用"."作为小数分隔符，没有千位分隔符，以","作为函数参数分隔符。
	*/
	void ParserBase::ResetLocale()
	{
		s_locale = std::locale(std::locale("C"), new change_dec_sep<char_type>('.'));
		SetArgSep(',');
	}

	//---------------------------------------------------------------------------
	/** \brief 初始化标记读取器。

	  创建新的标记读取器对象并提交函数、运算符、常量和变量定义的指针。

	  \post m_pTokenReader.get()!=0
	  \throw nothrow
	*/
	void ParserBase::InitTokenReader()
	{
		m_pTokenReader.reset(new token_reader_type(this));
	}

	//---------------------------------------------------------------------------
	/** \brief 重新初始化解析器为字符串解析模式并清除内部缓冲区。

		清除字节码，重置标记读取器。
		\throw nothrow
	*/
	void ParserBase::ReInit() const
	{
		m_pParseFormula = &ParserBase::ParseString;
		m_vStringBuf.clear();
		m_vRPN.clear();
		m_pTokenReader->ReInit();
	}

	//---------------------------------------------------------------------------
	void ParserBase::OnDetectVar(string_type * /*pExpr*/, int & /*nStart*/, int & /*nEnd*/)
	{
	}

	//---------------------------------------------------------------------------
	/** \brief 返回当前表达式的字节码。
	 */
	const ParserByteCode &ParserBase::GetByteCode() const
	{
		return m_vRPN;
	}

	//---------------------------------------------------------------------------
	/** \brief 返回muparser的版本。
		\param eInfo 一个标志，指示是否返回完整的版本信息。

	  格式如下："MAJOR.MINOR (COMPILER_FLAGS)" 仅当eInfo==pviFULL时返回COMPILER_FLAGS。
	*/
	string_type ParserBase::GetVersion(EParserVersionInfo eInfo) const
	{
		stringstream_type ss;

		ss << ParserVersion;

		if (eInfo == pviFULL)
		{
			ss << _T(" (") << ParserVersionDate;
			ss << std::dec << _T("; ") << sizeof(void *) * 8 << _T("BIT");

#ifdef _DEBUG
			ss << _T("; DEBUG");
#else
			ss << _T("; RELEASE");
#endif

#ifdef _UNICODE
			ss << _T("; UNICODE");
#else
#ifdef _MBCS
			ss << _T("; MBCS");
#else
			ss << _T("; ASCII");
#endif
#endif

#ifdef MUP_USE_OPENMP
			ss << _T("; OPENMP");
#endif

			ss << _T(")");
		}

		return ss.str();
	}
	// 该程序是一个数学表达式解析器，用于解析和计算数学表达式。这段代码是该解析器的一部分，实现了以下功能：

	// SetDecSep函数用于设置小数分隔符，覆盖了默认的区域设置。
	// SetThousandsSep函数用于设置千位分隔符，覆盖了默认的区域设置。
	// ResetLocale函数用于重置区域设置为默认设置，使用"."作为小数分隔符，没有千位分隔符，以","作为函数参数分隔符。
	// InitTokenReader函数用于初始化标记读取器对象。
	// ReInit函数用于重新初始化解析器为字符串解析模式并清除内部缓冲区。
	// OnDetectVar函数用于检测变量。
	// GetByteCode函数用于返回当前表达式的字节码。
	// GetVersion函数用于返回muparser的版本信息，包括版本号和编译器标志。
	//---------------------------------------------------------------------------
	/** \brief 添加值解析函数。
			在解析表达式时，muParser尝试使用不同的有效回调函数来检测表达式字符串中的值。
		因此，可以解析十六进制值、二进制值和浮点值。
	*/
	void ParserBase::AddValIdent(identfun_type a_pCallback)
	{
		m_pTokenReader->AddValIdent(a_pCallback);
	}

	//---------------------------------------------------------------------------
	/** \brief 设置一个函数，用于为未知的表达式变量创建变量指针。
		\param a_pFactory 变量工厂的指针。
		\param pUserData 用户定义的上下文指针。
	*/
	void ParserBase::SetVarFactory(facfun_type a_pFactory, void *pUserData)
	{
		m_pTokenReader->SetVarCreator(a_pFactory, pUserData);
	}

	//---------------------------------------------------------------------------
	/** \brief 向解析器添加函数或运算符回调函数。 */
	void ParserBase::AddCallback(
		const string_type &a_strName,
		const ParserCallback &a_Callback,
		funmap_type &a_Storage,
		const char_type *a_szCharSet)
	{
		if (!a_Callback.IsValid())
			Error(ecINVALID_FUN_PTR);

		const funmap_type *pFunMap = &a_Storage;

		// 检查运算符或函数名是否冲突
		if (pFunMap != &m_FunDef && m_FunDef.find(a_strName) != m_FunDef.end())
			Error(ecNAME_CONFLICT, -1, a_strName);

		if (pFunMap != &m_PostOprtDef && m_PostOprtDef.find(a_strName) != m_PostOprtDef.end())
			Error(ecNAME_CONFLICT, -1, a_strName);

		if (pFunMap != &m_InfixOprtDef && pFunMap != &m_OprtDef && m_InfixOprtDef.find(a_strName) != m_InfixOprtDef.end())
			Error(ecNAME_CONFLICT, -1, a_strName);

		if (pFunMap != &m_InfixOprtDef && pFunMap != &m_OprtDef && m_OprtDef.find(a_strName) != m_OprtDef.end())
			Error(ecNAME_CONFLICT, -1, a_strName);

		CheckOprt(a_strName, a_Callback, a_szCharSet);
		a_Storage[a_strName] = a_Callback;
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief 检查名称是否包含无效字符。

		\throw ParserException 如果名称包含无效字符。
	*/
	void ParserBase::CheckOprt(const string_type &a_sName,
							   const ParserCallback &a_Callback,
							   const string_type &a_szCharSet) const
	{
		if (!a_sName.length() ||
			(a_sName.find_first_not_of(a_szCharSet) != string_type::npos) ||
			(a_sName[0] >= '0' && a_sName[0] <= '9'))
		{
			switch (a_Callback.GetCode())
			{
			case cmOPRT_POSTFIX:
				Error(ecINVALID_POSTFIX_IDENT, -1, a_sName);
				break;
			case cmOPRT_INFIX:
				Error(ecINVALID_INFIX_IDENT, -1, a_sName);
				break;
			default:
				Error(ecINVALID_NAME, -1, a_sName);
				break;
			}
		}
	}

	/** \brief 检查名称是否包含无效字符。
		\throw ParserException 如果名称包含无效字符。
	*/
	void ParserBase::CheckName(const string_type &a_sName, const string_type &a_szCharSet) const
	{
		if (!a_sName.length() ||
			(a_sName.find_first_not_of(a_szCharSet) != string_type::npos) ||
			(a_sName[0] >= '0' && a_sName[0] <= '9'))
		{
			Error(ecINVALID_NAME);
		}
	}

	/** \brief 设置公式。
		\param a_strFormula 公式字符串
		\throw ParserException 如果存在语法错误。

		首次计算，创建字节码并扫描使用的变量。
	*/
	void ParserBase::SetExpr(const string_type &a_sExpr)
	{
		// 检查区域设置兼容性
		if (m_pTokenReader->GetArgSep() == std::use_facet<numpunct<char_type>>(s_locale).decimal_point())
			Error(ecLOCALE);

		// 检查允许的最大表达式长度。一个足够小的任意值，以便我可以调试发送给我的表达式
		if (a_sExpr.length() >= MaxLenExpression)
			Error(ecEXPRESSION_TOO_LONG, 0, a_sExpr);

		m_pTokenReader->SetFormula(a_sExpr + _T(" "));
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief 获取内置运算符使用的默认符号集。
		\sa c_DefaultOprt
	*/
	const char_type **ParserBase::GetOprtDef() const
	{
		return (const char_type **)(&c_DefaultOprt[0]);
	}

	//---------------------------------------------------------------------------
	/** \brief 定义函数、变量、常量名称中可使用的有效字符集。
	 */
	void ParserBase::DefineNameChars(const char_type *a_szCharset)
	{
		m_sNameChars = a_szCharset;
	}

	//---------------------------------------------------------------------------
	/** \brief 定义二元运算符和后缀运算符名称中可使用的有效字符集。
	 */
	void ParserBase::DefineOprtChars(const char_type *a_szCharset)
	{
		m_sOprtChars = a_szCharset;
	}
	// 代码实现了一个解析器的基本功能，包括添加值解析函数、设置变量工厂、添加函数或运算符回调函数、检查名称和运算符是否合法、设置公式以及定义有效字符集等
	//---------------------------------------------------------------------------
	/** \brief 定义用于中缀运算符名称的有效字符集合。
	 */
	void ParserBase::DefineInfixOprtChars(const char_type *a_szCharset)
	{
		m_sInfixOprtChars = a_szCharset;
	}

	//---------------------------------------------------------------------------
	/** \brief 虚函数，定义名称标识符中允许的字符。
	\sa #ValidOprtChars, #ValidPrefixOprtChars
	*/
	const char_type *ParserBase::ValidNameChars() const
	{
		MUP_ASSERT(m_sNameChars.size());
		return m_sNameChars.c_str();
	}

	//---------------------------------------------------------------------------
	/** \brief 虚函数，定义操作符定义中允许的字符。
	\sa #ValidNameChars, #ValidPrefixOprtChars
	*/
	const char_type *ParserBase::ValidOprtChars() const
	{
		MUP_ASSERT(m_sOprtChars.size());
		return m_sOprtChars.c_str();
	}

	//---------------------------------------------------------------------------
	/** \brief 虚函数，定义中缀操作符定义中允许的字符。
	\sa #ValidNameChars, #ValidOprtChars
	*/
	const char_type *ParserBase::ValidInfixOprtChars() const
	{
		MUP_ASSERT(m_sInfixOprtChars.size());
		return m_sInfixOprtChars.c_str();
	}

	//---------------------------------------------------------------------------
	/** \brief 添加用户定义的后缀操作符。
	\post 将解析器重置为字符串解析模式。
	*/
	void ParserBase::DefinePostfixOprt(const string_type &a_sName, fun_type1 a_pFun, bool a_bAllowOpt)
	{
		if (a_sName.length() > MaxLenIdentifier)
			Error(ecIDENTIFIER_TOO_LONG);

		AddCallback(a_sName, ParserCallback(a_pFun, a_bAllowOpt, prPOSTFIX, cmOPRT_POSTFIX), m_PostOprtDef, ValidOprtChars());
	}

	//---------------------------------------------------------------------------
	/** \brief 初始化用户定义的函数。

	调用虚函数InitFun()，InitConst()和InitOprt()。
	*/
	void ParserBase::Init()
	{
		InitCharSets();
		InitFun();
		InitConst();
		InitOprt();
	}

	//---------------------------------------------------------------------------
	/** \brief 添加用户定义的中缀操作符。
	\post 将解析器重置为字符串解析模式。
	\param [in] a_sName 操作符标识符
	\param [in] a_pFun 操作符回调函数
	\param [in] a_iPrec 操作符优先级（默认值=prSIGN）
	\param [in] a_bAllowOpt 如果操作符是易变的，则为true（默认值=false）
	\sa EPrec
	*/
	void ParserBase::DefineInfixOprt(const string_type &a_sName, fun_type1 a_pFun, int a_iPrec, bool a_bAllowOpt)
	{
		if (a_sName.length() > MaxLenIdentifier)
			Error(ecIDENTIFIER_TOO_LONG);
		AddCallback(a_sName, ParserCallback(a_pFun, a_bAllowOpt, a_iPrec, cmOPRT_INFIX), m_InfixOprtDef, ValidInfixOprtChars());
	}

	//---------------------------------------------------------------------------
	/** \brief 定义二元操作符。
	\param [in] a_sName 操作符的标识符。
	\param [in] a_pFun 回调函数的指针。
	\param [in] a_iPrec 操作符的优先级。
	\param [in] a_eAssociativity 操作符的结合性。
	\param [in] a_bAllowOpt 如果为true，则可以优化操作符。
	向解析器实例添加一个新的二元操作符。
	*/
	void ParserBase::DefineOprt(const string_type &a_sName, fun_type2 a_pFun, unsigned a_iPrec, EOprtAssociativity a_eAssociativity, bool a_bAllowOpt)
	{
		if (a_sName.length() > MaxLenIdentifier)
			Error(ecIDENTIFIER_TOO_LONG);
		// 检查与内置操作符名称的冲突
		for (int i = 0; m_bBuiltInOp && i < cmENDIF; ++i)
		{
			if (a_sName == string_type(c_DefaultOprt[i]))
			{
				Error(ecBUILTIN_OVERLOAD, -1, a_sName);
			}
		}

		AddCallback(a_sName, ParserCallback(a_pFun, a_bAllowOpt, a_iPrec, a_eAssociativity), m_OprtDef, ValidOprtChars());
	}

	//---------------------------------------------------------------------------
	/** \brief 定义新的字符串常量。
	\param [in] a_strName 常量的名称。
	\param [in] a_strVal 常量的值。
	*/
	void ParserBase::DefineStrConst(const string_type &a_strName, const string_type &a_strVal)
	{
		// 检查该名称的常量是否已经存在
		if (m_StrVarDef.find(a_strName) != m_StrVarDef.end())
			Error(ecNAME_CONFLICT);
		CheckName(a_strName, ValidNameChars());

		m_vStringVarBuf.push_back(a_strVal);				 // 将变量字符串存储在内部缓冲区中
		m_StrVarDef[a_strName] = m_vStringVarBuf.size() - 1; // 将缓冲区索引绑定到变量名称

		ReInit();
	} //---------------------------------------------------------------------------
	  /** \brief 添加用户定义的变量。
  \param [in] a_sName 变量名称
  \param [in] a_pVar 指向变量值的指针。
  \post 将解析器重置为字符串解析模式。
  \throw ParserException 如果名称包含无效字符或a_pVar为nullptr。
  */
	void ParserBase::DefineVar(const string_type &a_sName, value_type *a_pVar)
	{
		if (a_pVar == 0)
			Error(ecINVALID_VAR_PTR);
		if (a_sName.length() > MaxLenIdentifier)
			Error(ecIDENTIFIER_TOO_LONG);

		// 检查是否已存在同名的常量
		if (m_ConstDef.find(a_sName) != m_ConstDef.end())
			Error(ecNAME_CONFLICT);

		CheckName(a_sName, ValidNameChars());
		m_VarDef[a_sName] = a_pVar;
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief 添加用户定义的常量。
	\param [in] a_sName 常量名称。
	\param [in] a_fVal 常量的值。
	\post 将解析器重置为字符串解析模式。
	\throw ParserException 如果名称包含无效字符。
	*/
	void ParserBase::DefineConst(const string_type &a_sName, value_type a_fVal)
	{
		if (a_sName.length() > MaxLenIdentifier)
			Error(ecIDENTIFIER_TOO_LONG);
		CheckName(a_sName, ValidNameChars());
		m_ConstDef[a_sName] = a_fVal;
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief 获取运算符优先级。
	\throw ParserException 如果a_Oprt不是运算符代码。
	*/
	int ParserBase::GetOprtPrecedence(const token_type &a_Tok) const
	{
		switch (a_Tok.GetCode())
		{
		// 内置运算符
		case cmEND:
			return -5;
		case cmARG_SEP:
			return -4;
		case cmASSIGN:
			return -1;
		case cmELSE:
		case cmIF:
			return 0;
		case cmLAND:
			return prLAND;
		case cmLOR:
			return prLOR;
		case cmLT:
		case cmGT:
		case cmLE:
		case cmGE:
		case cmNEQ:
		case cmEQ:
			return prCMP;
		case cmADD:
		case cmSUB:
			return prADD_SUB;
		case cmMUL:
		case cmDIV:
			return prMUL_DIV;
		case cmPOW:
			return prPOW; // 用户定义的二进制运算符
		case cmOPRT_INFIX:
		case cmOPRT_BIN:
			return a_Tok.GetPri();
		default:
			throw exception_type(ecINTERNAL_ERROR, 5, _T(""));
		}
	}
	// 程序实现了一个解析器类，其中包含了添加用户定义变量和常量的功能，以及获取运算符优先级的功能。
	//---------------------------------------------------------------------------
	/** \brief Get operator priority.
		\throw ParserException if a_Oprt is no operator code
	*/
	EOprtAssociativity ParserBase::GetOprtAssociativity(const token_type &a_Tok) const
	{
		switch (a_Tok.GetCode())
		{
		case cmASSIGN:
		case cmLAND:
		case cmLOR:
		case cmLT:
		case cmGT:
		case cmLE:
		case cmGE:
		case cmNEQ:
		case cmEQ:
		case cmADD:
		case cmSUB:
		case cmMUL:
		case cmDIV:
			return oaLEFT;
		case cmPOW:
			return oaRIGHT;
		case cmOPRT_BIN:
			return a_Tok.GetAssociativity();
		default:
			return oaNONE;
		}
	}
	//---------------------------------------------------------------------------
	/** \brief 返回一个仅包含已使用变量的映射表。 */
	const varmap_type &ParserBase::GetUsedVar() const
	{
		try
		{
			m_pTokenReader->IgnoreUndefVar(true);
			CreateRPN(); // 尝试创建字节码，但不用于进一步的计算，因为它可能引用不存在的变量。
			m_pParseFormula = &ParserBase::ParseString;
			m_pTokenReader->IgnoreUndefVar(false);
		}
		catch (exception_type & /*e*/)
		{
			// 确保保持在字符串解析模式中，不调用ReInit()，因为它会删除用于的变量数组
			m_pParseFormula = &ParserBase::ParseString;
			m_pTokenReader->IgnoreUndefVar(false);
			throw;
		}

		return m_pTokenReader->GetUsedVar();
	}

	//---------------------------------------------------------------------------
	/** \brief 返回一个仅包含已定义变量的映射表。 */
	const varmap_type &ParserBase::GetVar() const
	{
		return m_VarDef;
	}

	//---------------------------------------------------------------------------
	/** \brief 返回一个包含所有解析器常量的映射表。 */
	const valmap_type &ParserBase::GetConst() const
	{
		return m_ConstDef;
	}

	//---------------------------------------------------------------------------
	/** \brief 返回所有解析器函数的原型。
		\return #m_FunDef
		\sa FunProt
		\throw nothrow

		返回类型是一个公共类型 #funmap_type 的映射表，其中包含所有数值解析器函数的原型定义。字符串函数不是该映射表的一部分。
		原型定义封装在类 FunProt 的对象中，每个解析器函数关联一个函数名通过一个映射结构。
	*/
	const funmap_type &ParserBase::GetFunDef() const
	{
		return m_FunDef;
	}

	//---------------------------------------------------------------------------
	/** \brief 检索公式。 */
	const string_type &ParserBase::GetExpr() const
	{
		return m_pTokenReader->GetExpr();
	}

	//---------------------------------------------------------------------------
	/** \brief 执行一个带有单个字符串参数的函数。
		\param a_FunTok 函数令牌。
		\throw exception_type 如果函数令牌不是一个字符串函数
	*/
	ParserBase::token_type ParserBase::ApplyStrFunc(
		const token_type &a_FunTok,
		const std::vector<token_type> &a_vArg) const
	{
		if (a_vArg.back().GetCode() != cmSTRING)
			Error(ecSTRING_EXPECTED, m_pTokenReader->GetPos(), a_FunTok.GetAsString());

		token_type valTok;
		generic_callable_type pFunc = a_FunTok.GetFuncAddr();
		MUP_ASSERT(pFunc);

		try
		{
			// 检查函数参数；将虚拟值写入valtok以表示结果
			switch (a_FunTok.GetArgCount())
			{
			case 0:
				valTok.SetVal(1);
				a_vArg[0].GetAsString();
				break;
			case 1:
				valTok.SetVal(1);
				a_vArg[1].GetAsString();
				a_vArg[0].GetVal();
				break;
			case 2:
				valTok.SetVal(1);
				a_vArg[2].GetAsString();
				a_vArg[1].GetVal();
				a_vArg[0].GetVal();
				break;
			case 3:
				valTok.SetVal(1);
				a_vArg[3].GetAsString();
				a_vArg[2].GetVal();
				a_vArg[1].GetVal();
				a_vArg[0].GetVal();
				break;
			case 4:
				valTok.SetVal(1);
				a_vArg[4].GetAsString();
				a_vArg[3].GetVal();
				a_vArg[2].GetVal();
				a_vArg[1].GetVal();
				a_vArg[0].GetVal();
				break;
			case 5:
				valTok.SetVal(1);
				a_vArg[5].GetAsString();
				a_vArg[4].GetVal();
				a_vArg[3].GetVal();
				a_vArg[2].GetVal();
				a_vArg[1].GetVal();
				a_vArg[0].GetVal();
				break;
			default:
				Error(ecINTERNAL_ERROR);
			}
		}
		catch (ParserError &)
		{
			Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), a_FunTok.GetAsString());
		}

		// 字符串函数不会被优化
		m_vRPN.AddStrFun(pFunc, a_FunTok.GetArgCount(), a_vArg.back().GetIdx());

		// 将表示函数结果的虚拟值推入栈中
		return valTok;
	}
	// 该程序实现的功能是一个解析器类，提供了一些用于解析和处理数学表达式的方法。具体功能如下：

	// GetUsedVar()：返回一个仅包含已使用变量的映射表。
	// GetVar()：返回一个仅包含已定义变量的映射表。
	// GetConst()：返回一个包含所有解析器常量的映射表。
	// GetFunDef()：返回所有解析器函数的原型。
	// GetExpr()：检索当前的数学表达式。
	// ApplyStrFunc()：执行一个带有单个字符串参数的函数。

	//---------------------------------------------------------------------------
	/** \brief 应用函数标记。
		\param iArgCount 实际收集到的参数数量，仅对多参数函数有效。
		\post 结果被推送到值栈中。
		\post 函数标记从栈中移除。
		\throw exception_type 如果参数数量与函数要求不匹配。
	*/
	void ParserBase::ApplyFunc(std::stack<token_type> &a_stOpt, std::stack<token_type> &a_stVal, int a_iArgCount) const
	{
		MUP_ASSERT(m_pTokenReader.get());

		// 操作符栈为空或不包含具有回调函数的标记
		if (a_stOpt.empty() || a_stOpt.top().GetFuncAddr() == 0)
			return;

		token_type funTok = a_stOpt.top();
		a_stOpt.pop();
		MUP_ASSERT(funTok.GetFuncAddr() != nullptr);

		// 二元运算符必须依赖于其内部运算符编号，因为运算符的计数依赖于函数参数中的逗号，而二元运算符的表达式中没有逗号
		int iArgCount = (funTok.GetCode() == cmOPRT_BIN) ? funTok.GetArgCount() : a_iArgCount;

		// 确定函数需要的参数数量。iArgCount包括字符串参数，而GetArgCount()仅计数数值参数。
		int iArgRequired = funTok.GetArgCount() + ((funTok.GetType() == tpSTR) ? 1 : 0);

		// 数值参数的数量
		int iArgNumerical = iArgCount - ((funTok.GetType() == tpSTR) ? 1 : 0);

		if (funTok.GetCode() == cmFUNC_STR && iArgCount - iArgNumerical > 1)
			Error(ecINTERNAL_ERROR);

		if (funTok.GetArgCount() >= 0 && iArgCount > iArgRequired)
			Error(ecTOO_MANY_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

		if (funTok.GetCode() != cmOPRT_BIN && iArgCount < iArgRequired)
			Error(ecTOO_FEW_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

		if (funTok.GetCode() == cmFUNC_STR && iArgCount > iArgRequired)
			Error(ecTOO_MANY_PARAMS, m_pTokenReader->GetPos() - 1, funTok.GetAsString());

		// 从值栈中收集数值函数参数并存储在向量中
		std::vector<token_type> stArg;
		for (int i = 0; i < iArgNumerical; ++i)
		{
			if (a_stVal.empty())
				Error(ecINTERNAL_ERROR, m_pTokenReader->GetPos(), funTok.GetAsString());

			stArg.push_back(a_stVal.top());
			a_stVal.pop();

			if (stArg.back().GetType() == tpSTR && funTok.GetType() != tpSTR)
				Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), funTok.GetAsString());
		}

		switch (funTok.GetCode())
		{
		case cmFUNC_STR:
			if (a_stVal.empty())
				Error(ecINTERNAL_ERROR, m_pTokenReader->GetPos(), funTok.GetAsString());

			stArg.push_back(a_stVal.top());
			a_stVal.pop();

			if (stArg.back().GetType() == tpSTR && funTok.GetType() != tpSTR)
				Error(ecVAL_EXPECTED, m_pTokenReader->GetPos(), funTok.GetAsString());

			ApplyStrFunc(funTok, stArg);
			break;

		case cmFUNC_BULK:
			m_vRPN.AddBulkFun(funTok.GetFuncAddr(), (int)stArg.size());
			break;

		case cmOPRT_BIN:
		case cmOPRT_POSTFIX:
		case cmOPRT_INFIX:
		case cmFUNC:
			if (funTok.GetArgCount() == -1 && iArgCount == 0)
				Error(ecTOO_FEW_PARAMS, m_pTokenReader->GetPos(), funTok.GetAsString());

			m_vRPN.AddFun(funTok.GetFuncAddr(), (funTok.GetArgCount() == -1) ? -iArgNumerical : iArgNumerical, funTok.IsOptimizable());
			break;
		default:
			break;
		}

		// 推送表示函数结果的虚拟值到栈中
		token_type token;
		token.SetVal(1);
		a_stVal.push(token);
	}

	//---------------------------------------------------------------------------
	void ParserBase::ApplyIfElse(std::stack<token_type> &a_stOpt, std::stack<token_type> &a_stVal) const
	{
		// 检查是否存在if-else子句需要计算
		while (a_stOpt.size() && a_stOpt.top().GetCode() == cmELSE)
		{
			MUP_ASSERT(!a_stOpt.empty())
			token_type opElse = a_stOpt.top();
			a_stOpt.pop();

			// 从值栈中取出与else分支关联的值
			MUP_ASSERT(!a_stVal.empty());
			token_type vVal2 = a_stVal.top();
			if (vVal2.GetType() != tpDBL)
				Error(ecUNEXPECTED_STR, m_pTokenReader->GetPos());

			a_stVal.pop();

			// 如果是三元运算符，则从值栈中弹出所有三个值，并返回右值
			MUP_ASSERT(!a_stVal.empty());
			token_type vVal1 = a_stVal.top();
			if (vVal1.GetType() != tpDBL)
				Error(ecUNEXPECTED_STR, m_pTokenReader->GetPos());

			a_stVal.pop();

			MUP_ASSERT(!a_stVal.empty());
			token_type vExpr = a_stVal.top();
			a_stVal.pop();

			a_stVal.push((vExpr.GetVal() != 0) ? vVal1 : vVal2);

			token_type opIf = a_stOpt.top();
			a_stOpt.pop();

			MUP_ASSERT(opElse.GetCode() == cmELSE);

			if (opIf.GetCode() != cmIF)
				Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());

			m_vRPN.AddIfElse(cmENDIF);
		} // while存在待处理的if-else子句
	}
	// 该程序实现了一个解析器（Parser），其中的 ApplyFunc 函数用于应用函数标记，根据给定的函数标记和参数，执行相应的函数操作，并将结果推送到值栈中。ApplyIfElse 函数用于处理 if-else 条件语句的计算。

	//---------------------------------------------------------------------------
	/** \brief 执行将二元操作符转换为字节码的必要步骤的函数。
	 */
	void ParserBase::ApplyBinOprt(std::stack<token_type> &a_stOpt, std::stack<token_type> &a_stVal) const
	{
		// 是否是用户定义的二元操作符？
		if (a_stOpt.top().GetCode() == cmOPRT_BIN)
		{
			ApplyFunc(a_stOpt, a_stVal, 2);
		}
		else
		{
			if (a_stVal.size() < 2)
				Error(ecINTERNAL_ERROR, m_pTokenReader->GetPos(), _T("ApplyBinOprt: value stack中的值不足!"));

			token_type valTok1 = a_stVal.top();
			a_stVal.pop();

			token_type valTok2 = a_stVal.top();
			a_stVal.pop();

			token_type optTok = a_stOpt.top();
			a_stOpt.pop();

			token_type resTok;

			if (valTok1.GetType() != valTok2.GetType() ||
				(valTok1.GetType() == tpSTR && valTok2.GetType() == tpSTR))
				Error(ecOPRT_TYPE_CONFLICT, m_pTokenReader->GetPos(), optTok.GetAsString());

			if (optTok.GetCode() == cmASSIGN)
			{
				if (valTok2.GetCode() != cmVAR)
					Error(ecUNEXPECTED_OPERATOR, -1, _T("="));

				m_vRPN.AddAssignOp(valTok2.GetVar());
			}
			else
				m_vRPN.AddOp(optTok.GetCode());

			resTok.SetVal(1);
			a_stVal.push(resTok);
		}
	}

	//---------------------------------------------------------------------------
	/** \brief 应用剩余的操作符。
		\param a_stOpt 操作符栈
		\param a_stVal 值栈
	*/
	void ParserBase::ApplyRemainingOprt(std::stack<token_type> &stOpt, std::stack<token_type> &stVal) const
	{
		while (stOpt.size() &&
			   stOpt.top().GetCode() != cmBO &&
			   stOpt.top().GetCode() != cmIF)
		{
			token_type tok = stOpt.top();
			switch (tok.GetCode())
			{
			case cmOPRT_INFIX:
			case cmOPRT_BIN:
			case cmLE:
			case cmGE:
			case cmNEQ:
			case cmEQ:
			case cmLT:
			case cmGT:
			case cmADD:
			case cmSUB:
			case cmMUL:
			case cmDIV:
			case cmPOW:
			case cmLAND:
			case cmLOR:
			case cmASSIGN:
				if (stOpt.top().GetCode() == cmOPRT_INFIX)
					ApplyFunc(stOpt, stVal, 1);
				else
					ApplyBinOprt(stOpt, stVal);
				break;

			case cmELSE:
				ApplyIfElse(stOpt, stVal);
				break;

			default:
				Error(ecINTERNAL_ERROR);
			}
		}
	}

	//---------------------------------------------------------------------------
	/** \brief 解析命令代码。
		\sa ParseString(...)

		命令代码包含预先计算的值和相关操作符的栈位置。栈从索引1开始填充，索引0处的值未被使用。
	*/
	value_type ParserBase::ParseCmdCode() const
	{
		return ParseCmdCodeBulk(0, 0);
	}

	value_type ParserBase::ParseCmdCodeShort() const
	{
		const SToken *const tok = m_vRPN.GetBase();
		value_type buf;

		switch (tok->Cmd)
		{
		case cmVAL:
			return tok->Val.data2;

		case cmVAR:
			return *tok->Val.ptr;

		case cmVARMUL:
			return *tok->Val.ptr * tok->Val.data + tok->Val.data2;

		case cmVARPOW2:
			buf = *(tok->Val.ptr);
			return buf * buf;

		case cmVARPOW3:
			buf = *(tok->Val.ptr);
			return buf * buf * buf;

		case cmVARPOW4:
			buf = *(tok->Val.ptr);
			return buf * buf * buf * buf;

		// 无参数的数值函数
		case cmFUNC:
			return tok->Fun.cb.call_fun<0>();

		// 无数值参数的字符串函数
		case cmFUNC_STR:
			return tok->Fun.cb.call_strfun<1>(m_vStringBuf[0].c_str());

		default:
			throw ParserError(ecINTERNAL_ERROR);
		}
	}
	// ApplyBinOprt函数执行了将二元操作符转换为字节码的必要步骤。
	// ApplyRemainingOprt函数应用了剩余的操作符。
	// ParseCmdCode函数解析了命令代码。
	// ParseCmdCodeShort函数解析了命令代码的缩写形式。
	/** \brief 评估逆波兰表示法（RPN）。
	\param nOffset 变量地址的偏移量（用于批量模式）
	\param nThreadID 调用线程的OpenMP线程ID
*/
	value_type ParserBase::ParseCmdCodeBulk(int nOffset, int nThreadID) const
	{
		assert(nThreadID <= s_MaxNumOpenMPThreads);

		// 注意：这里对nOffset和nThreadID进行检查并不是必需的，但在非批量模式下可以带来轻微的性能提升。
		value_type *stack = ((nOffset == 0) && (nThreadID == 0)) ? &m_vStackBuffer[0] : &m_vStackBuffer[nThreadID * (m_vStackBuffer.size() / s_MaxNumOpenMPThreads)];
		value_type buf;
		int sidx(0);
		for (const SToken *pTok = m_vRPN.GetBase(); pTok->Cmd != cmEND; ++pTok)
		{
			switch (pTok->Cmd)
			{
			// 内置二元运算符
			case cmLE:
				--sidx;
				stack[sidx] = stack[sidx] <= stack[sidx + 1];
				continue;
			case cmGE:
				--sidx;
				stack[sidx] = stack[sidx] >= stack[sidx + 1];
				continue;
			case cmNEQ:
				--sidx;
				stack[sidx] = stack[sidx] != stack[sidx + 1];
				continue;
			case cmEQ:
				--sidx;
				stack[sidx] = stack[sidx] == stack[sidx + 1];
				continue;
			case cmLT:
				--sidx;
				stack[sidx] = stack[sidx] < stack[sidx + 1];
				continue;
			case cmGT:
				--sidx;
				stack[sidx] = stack[sidx] > stack[sidx + 1];
				continue;
			case cmADD:
				--sidx;
				stack[sidx] += stack[1 + sidx];
				continue;
			case cmSUB:
				--sidx;
				stack[sidx] -= stack[1 + sidx];
				continue;
			case cmMUL:
				--sidx;
				stack[sidx] *= stack[1 + sidx];
				continue;
			case cmDIV:
				--sidx;
				stack[sidx] /= stack[1 + sidx];
				continue;

			case cmPOW:
				--sidx;
				stack[sidx] = MathImpl<value_type>::Pow(stack[sidx], stack[1 + sidx]);
				continue;

			case cmLAND:
				--sidx;
				stack[sidx] = stack[sidx] && stack[sidx + 1];
				continue;
			case cmLOR:
				--sidx;
				stack[sidx] = stack[sidx] || stack[sidx + 1];
				continue;

			case cmASSIGN:
				// Bugfix for Bulkmode:
				// for details see:
				//    https://groups.google.com/forum/embed/?place=forum/muparser-dev&showsearch=true&showpopout=true&showtabs=false&parenturl=http://muparser.beltoforion.de/mup_forum.html&afterlogin&pli=1#!topic/muparser-dev/szgatgoHTws
				--sidx;
				stack[sidx] = *(pTok->Oprt.ptr + nOffset) = stack[sidx + 1];
				continue;
				// original code:
				//--sidx; Stack[sidx] = *pTok->Oprt.ptr = Stack[sidx+1]; continue;

			case cmIF:
				if (stack[sidx--] == 0)
				{
					MUP_ASSERT(sidx >= 0);
					pTok += pTok->Oprt.offset;
				}
				continue;

			case cmELSE:
				pTok += pTok->Oprt.offset;
				continue;

			case cmENDIF:
				continue;

			// 值和变量标记
			case cmVAR:
				stack[++sidx] = *(pTok->Val.ptr + nOffset);
				continue;
			case cmVAL:
				stack[++sidx] = pTok->Val.data2;
				continue;

			case cmVARPOW2:
				buf = *(pTok->Val.ptr + nOffset);
				stack[++sidx] = buf * buf;
				continue;

			case cmVARPOW3:
				buf = *(pTok->Val.ptr + nOffset);
				stack[++sidx] = buf * buf * buf;
				continue;

			case cmVARPOW4:
				buf = *(pTok->Val.ptr + nOffset);
				stack[++sidx] = buf * buf * buf * buf;
				continue;

			case cmVARMUL:
				stack[++sidx] = *(pTok->Val.ptr + nOffset) * pTok->Val.data + pTok->Val.data2;
				continue;

			// 接下来处理数值函数
			case cmFUNC:
			{
				int iArgCount = pTok->Fun.argc;

				// 根据参数数量进行切换
				switch (iArgCount)
				{
				case 0:
					sidx += 1;
					stack[sidx] = pTok->Fun.cb.call_fun<0>();
					continue;
				case 1:
					stack[sidx] = pTok->Fun.cb.call_fun<1>(stack[sidx]);
					continue;
				case 2:
					sidx -= 1;
					stack[sidx] = pTok->Fun.cb.call_fun<2>(stack[sidx], stack[sidx + 1]);
					continue;
				case 3:
					sidx -= 2;
					stack[sidx] = pTok->Fun.cb.call_fun<3>(stack[sidx], stack[sidx + 1], stack[sidx + 2]);
					continue;
				case 4:
					sidx -= 3;
					stack[sidx] = pTok->Fun.cb.call_fun<4>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3]);
					continue;
				case 5:
					sidx -= 4;
					stack[sidx] = pTok->Fun.cb.call_fun<5>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4]);
					continue;
				case 6:
					sidx -= 5;
					stack[sidx] = pTok->Fun.cb.call_fun<6>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5]);
					continue;
				case 7:
					sidx -= 6;
					stack[sidx] = pTok->Fun.cb.call_fun<7>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6]);
					continue;
				case 8:
					sidx -= 7;
					stack[sidx] = pTok->Fun.cb.call_fun<8>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6], stack[sidx + 7]);
					continue;
				case 9:
					sidx -= 8;
					stack[sidx] = pTok->Fun.cb.call_fun<9>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6], stack[sidx + 7], stack[sidx + 8]);
					continue;
				case 10:
					sidx -= 9;
					stack[sidx] = pTok->Fun.cb.call_fun<10>(stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6], stack[sidx + 7], stack[sidx + 8], stack[sidx + 9]);
					continue;
				default:
					// 变量参数的函数将数量作为负值存储
					if (iArgCount > 0)
						Error(ecINTERNAL_ERROR, -1);

					sidx -= -iArgCount - 1;

					// <ibg 2020-06-08> 来自oss-fuzz。当多参数函数和if-then-else使用不正确时发生。观察到这种情况的表达式有：
					//		sum(0?1,2,3,4,5)			-> 已修复
					//		avg(0>3?4:(""),0^3?4:(""))
					//
					// 最终结果通常在位置1。如果sidx较小，则出现错误。
					if (sidx <= 0)
						Error(ecINTERNAL_ERROR, -1);
					// </ibg>

					stack[sidx] = pTok->Fun.cb.call_multfun(&stack[sidx], -iArgCount);
					continue;
				}
			}

			// 程序实现了解析逆波兰表达式并进行计算的功能

			// 下面是对字符串函数的处理
			case cmFUNC_STR:
			{
				sidx -= pTok->Fun.argc - 1;

				// 字符串参数在字符串表中的索引
				int iIdxStack = pTok->Fun.idx;
				if (iIdxStack < 0 || iIdxStack >= (int)m_vStringBuf.size())
					Error(ecINTERNAL_ERROR, m_pTokenReader->GetPos());

				switch (pTok->Fun.argc) // 根据参数数量进行切换
				{
				case 0:
					stack[sidx] = pTok->Fun.cb.call_strfun<1>(m_vStringBuf[iIdxStack].c_str());
					continue;
				case 1:
					stack[sidx] = pTok->Fun.cb.call_strfun<2>(m_vStringBuf[iIdxStack].c_str(), stack[sidx]);
					continue;
				case 2:
					stack[sidx] = pTok->Fun.cb.call_strfun<3>(m_vStringBuf[iIdxStack].c_str(), stack[sidx], stack[sidx + 1]);
					continue;
				case 3:
					stack[sidx] = pTok->Fun.cb.call_strfun<4>(m_vStringBuf[iIdxStack].c_str(), stack[sidx], stack[sidx + 1], stack[sidx + 2]);
					continue;
				case 4:
					stack[sidx] = pTok->Fun.cb.call_strfun<5>(m_vStringBuf[iIdxStack].c_str(), stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3]);
					continue;
				case 5:
					stack[sidx] = pTok->Fun.cb.call_strfun<6>(m_vStringBuf[iIdxStack].c_str(), stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4]);
					continue;
				}

				continue;
			}

			case cmFUNC_BULK:
			{
				int iArgCount = pTok->Fun.argc;

				// 根据参数数量进行切换
				switch (iArgCount)
				{
				case 0:
					sidx += 1;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<0>(nOffset, nThreadID);
					continue;
				case 1:
					stack[sidx] = pTok->Fun.cb.call_bulkfun<1>(nOffset, nThreadID, stack[sidx]);
					continue;
				case 2:
					sidx -= 1;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<2>(nOffset, nThreadID, stack[sidx], stack[sidx + 1]);
					continue;
				case 3:
					sidx -= 2;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<3>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2]);
					continue;
				case 4:
					sidx -= 3;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<4>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3]);
					continue;
				case 5:
					sidx -= 4;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<5>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4]);
					continue;
				case 6:
					sidx -= 5;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<6>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5]);
					continue;
				case 7:
					sidx -= 6;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<7>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6]);
					continue;
				case 8:
					sidx -= 7;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<8>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6], stack[sidx + 7]);
					continue;
				case 9:
					sidx -= 8;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<9>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6], stack[sidx + 7], stack[sidx + 8]);
					continue;
				case 10:
					sidx -= 9;
					stack[sidx] = pTok->Fun.cb.call_bulkfun<10>(nOffset, nThreadID, stack[sidx], stack[sidx + 1], stack[sidx + 2], stack[sidx + 3], stack[sidx + 4], stack[sidx + 5], stack[sidx + 6], stack[sidx + 7], stack[sidx + 8], stack[sidx + 9]);
					continue;
				default:
					throw exception_type(ecINTERNAL_ERROR, 2, _T(""));
				}
			}

			default:
				throw exception_type(ecINTERNAL_ERROR, 3, _T(""));
			} // switch CmdCode
		}	  // for all bytecode tokens

		return stack[m_nFinalResultIdx];
	}

	void ParserBase::CreateRPN() const
	{
		if (!m_pTokenReader->GetExpr().length())
			Error(ecUNEXPECTED_EOF, 0);

		std::stack<token_type> stOpt, stVal; // 运算符栈、数值栈
		std::stack<int> stArgCount;			 // 参数计数栈
		token_type opta, opt;				 // 用于存储运算符
		token_type val, tval;				 // 用于存储数值
		int ifElseCounter = 0;				 // if-else计数器

		ReInit();

		stArgCount.push(1); // 将参数计数1压入栈顶，用于记录分隔的项的数量，如"a=10,b=20,c=c+a"

		for (;;)
		{
			opt = m_pTokenReader->ReadNextToken(); // 读取下一个token

			switch (opt.GetCode())
			{
			// 下面的三个case分别处理不同类型的值
			case cmSTRING: // 字符串值
				if (stOpt.empty())
					Error(ecSTR_RESULT, m_pTokenReader->GetPos(), opt.GetAsString());

				opt.SetIdx((int)m_vStringBuf.size()); // 将字符串存储在内部缓冲区，并将缓冲区索引分配给token
				stVal.push(opt);
				m_vStringBuf.push_back(opt.GetAsString()); // 将字符串存储在内部缓冲区中
				break;

			case cmVAR: // 变量
				stVal.push(opt);
				m_vRPN.AddVar(static_cast<value_type *>(opt.GetVar()));
				break;

			case cmVAL: // 数值
				stVal.push(opt);
				m_vRPN.AddVal(opt.GetVal());
				break;

			case cmELSE: // else关键字
				if (stArgCount.empty())
					Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());

				if (stArgCount.top() > 1)
					Error(ecUNEXPECTED_ARG_SEP, m_pTokenReader->GetPos());

				stArgCount.pop();

				ifElseCounter--;
				if (ifElseCounter < 0)
					Error(ecMISPLACED_COLON, m_pTokenReader->GetPos());

				ApplyRemainingOprt(stOpt, stVal);
				m_vRPN.AddIfElse(cmELSE);
				stOpt.push(opt);
				break;

			case cmARG_SEP: // 参数分隔符
				if (!stOpt.empty() && stOpt.top().GetCode() == cmIF)
					Error(ecUNEXPECTED_ARG_SEP, m_pTokenReader->GetPos());

				if (stArgCount.empty())
					Error(ecUNEXPECTED_ARG_SEP, m_pTokenReader->GetPos());

				++stArgCount.top();
				// Falls through.
				// intentional (no break!)

			case cmEND: // 表达式结束符
				ApplyRemainingOprt(stOpt, stVal);
				break;

			case cmBC: // 括号闭合
			{
				// 无参数函数的参数计数为0
				// 开括号默认将参数计数设置为1，为接下来的参数做准备
				// 如果上一个token是开括号，则参数计数减1
				if (opta.GetCode() == cmBO)
					--stArgCount.top();

				ApplyRemainingOprt(stOpt, stVal);

				// 检查括号内的内容是否完全计算完成
				if (stOpt.size() && stOpt.top().GetCode() == cmBO)
				{
					// 如果opt是")"且opta是"("，则括号已经计算完成，现在是时候检查
					// 是否有待处理的函数或运算符
					// 开括号和闭括号都不会被推回运算符栈
					// 检查开括号前面是否有一个函数，
					// 如果有，先计算函数，然后再检查是否有中缀运算符
					MUP_ASSERT(stArgCount.size());
					int iArgCount = stArgCount.top();
					stArgCount.pop();

					stOpt.pop(); // 从栈中弹出开括号

					if (iArgCount > 1 && (stOpt.size() == 0 ||
										  (stOpt.top().GetCode() != cmFUNC &&
										   stOpt.top().GetCode() != cmFUNC_BULK &&
										   stOpt.top().GetCode() != cmFUNC_STR)))
						Error(ecUNEXPECTED_ARG, m_pTokenReader->GetPos());

					// 开括号已从栈中弹出，现在检查此括号之前是否有函数
					if (stOpt.size() &&
						stOpt.top().GetCode() != cmOPRT_INFIX &&
						stOpt.top().GetCode() != cmOPRT_BIN &&
						stOpt.top().GetFuncAddr() != 0)
					{
						ApplyFunc(stOpt, stVal, iArgCount);
					}
				}
			} // if bracket content is evaluated
			break;

			// 下面的case处理二元运算符
			case cmIF: // if关键字
				ifElseCounter++;
				stArgCount.push(1);
				// Falls through.
				// intentional (no break!)

			case cmLAND:	 // 逻辑与
			case cmLOR:		 // 逻辑或
			case cmLT:		 // 小于
			case cmGT:		 // 大于
			case cmLE:		 // 小于等于
			case cmGE:		 // 大于等于
			case cmNEQ:		 // 不等于
			case cmEQ:		 // 等于
			case cmADD:		 // 加法
			case cmSUB:		 // 减法
			case cmMUL:		 // 乘法
			case cmDIV:		 // 除法
			case cmPOW:		 // 幂运算
			case cmASSIGN:	 // 赋值
			case cmOPRT_BIN: // 二元运算符
				// 发现二元运算符（用户定义或内置）
				while (
					stOpt.size() &&
					stOpt.top().GetCode() != cmBO &&
					stOpt.top().GetCode() != cmELSE &&
					stOpt.top().GetCode() != cmIF)
				{
					int nPrec1 = GetOprtPrecedence(stOpt.top()),
						nPrec2 = GetOprtPrecedence(opt);

					if (stOpt.top().GetCode() == opt.GetCode())
					{
						// 处理运算符的结合性
						EOprtAssociativity eOprtAsct = GetOprtAssociativity(opt);
						if ((eOprtAsct == oaRIGHT && (nPrec1 <= nPrec2)) ||
							(eOprtAsct == oaLEFT && (nPrec1 < nPrec2)))
						{
							break;
						}
					}
					else if (nPrec1 < nPrec2)
					{
						// 如果运算符不相等，优先级决定...
						break;
					}

					if (stOpt.top().GetCode() == cmOPRT_INFIX)
						ApplyFunc(stOpt, stVal, 1);
					else
						ApplyBinOprt(stOpt, stVal);
				} // while ( ... )

				if (opt.GetCode() == cmIF)
					m_vRPN.AddIfElse(opt.GetCode());

				// 无法立即计算运算符，将其推回运算符栈
				stOpt.push(opt);
				break;

				// 最后一部分包含隐式映射为函数的函数和运算符
			case cmBO: // 开括号
				stArgCount.push(1);
				stOpt.push(opt);
				break;

			case cmOPRT_INFIX: // 中缀运算符
			case cmFUNC:	   // 函数
			case cmFUNC_BULK:  // 批量函数
			case cmFUNC_STR:   // 字符串函数
				stOpt.push(opt);
				break;

			case cmOPRT_POSTFIX: // 后缀运算符
				stOpt.push(opt);
				ApplyFunc(stOpt, stVal, 1); // 这是后缀运算符
				break;

			default:
				Error(ecINTERNAL_ERROR, 3);
			} // end of switch operator-token

			opta = opt;

			if (opt.GetCode() == cmEND)
			{
				m_vRPN.Finalize();
				break;
			}

			if (ParserBase::g_DbgDumpStack)
			{
				StackDump(stVal, stOpt);
				m_vRPN.AsciiDump();
			}

			//			if (ParserBase::g_DbgDumpCmdCode)
			// m_vRPN.AsciiDump();
		} // while (true)

		if (ParserBase::g_DbgDumpCmdCode)
			m_vRPN.AsciiDump();

		if (ifElseCounter > 0)
			Error(ecMISSING_ELSE_CLAUSE);

		// 从栈中获取最后一个值（最终结果）
		MUP_ASSERT(stArgCount.size() == 1);
		m_nFinalResultIdx = stArgCount.top();
		if (m_nFinalResultIdx == 0)
			Error(ecINTERNAL_ERROR, 9);

		if (stVal.size() == 0)
			Error(ecEMPTY_EXPRESSION);

		// 2020-09-17; fix for https://oss-fuzz.com/testcase-detail/5758791700971520
		// 我不再需要值栈。破坏性地检查值栈中的所有值是否都表示浮点数值
		while (stVal.size())
		{
			if (stVal.top().GetType() != tpDBL)
				Error(ecSTR_RESULT);

			stVal.pop();
		}

		m_vStackBuffer.resize(m_vRPN.GetMaxStackSize() * s_MaxNumOpenMPThreads);
	}

	// 程序实现了创建逆波兰表达式（RPN）的功能。

	//---------------------------------------------------------------------------
	/** \brief One of the two main parse functions.
		\sa ParseCmdCode(...)

	  解析输入字符串中的表达式。执行语法检查并创建字节码。在解析字符串并创建字节码之后，函数指针 #m_pParseFormula 将被更改为使用字节码而不是字符串解析的第二个解析函数。
	*/
	value_type ParserBase::ParseString() const
	{
		try
		{
			CreateRPN();

			if (m_vRPN.GetSize() == 2)
			{
				m_pParseFormula = &ParserBase::ParseCmdCodeShort;
				m_vStackBuffer[1] = (this->*m_pParseFormula)();
				return m_vStackBuffer[1];
			}
			else
			{
				m_pParseFormula = &ParserBase::ParseCmdCode;
				return (this->*m_pParseFormula)();
			}
		}
		catch (ParserError &exc)
		{
			exc.SetFormula(m_pTokenReader->GetExpr());
			throw;
		}
	}

	//---------------------------------------------------------------------------
	/** \brief Create an error containing the parse error position.

	  此函数将创建一个包含错误文本和其位置的解析器异常对象。

	  \param a_iErrc [in] 错误代码，类型为 #EErrorCodes。
	  \param a_iPos [in] 错误位置。
	  \param a_strTok [in] 与错误相关联的令牌字符串表示。
	  \throw ParserException 总是抛出异常，这是此函数的唯一目的。
	*/
	void ParserBase::Error(EErrorCodes a_iErrc, int a_iPos, const string_type &a_sTok) const
	{
		throw exception_type(a_iErrc, a_sTok, m_pTokenReader->GetExpr(), a_iPos);
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined variables.
		\throw nothrow

		通过调用 #ReInit 将解析器重置为字符串解析模式。
	*/
	void ParserBase::ClearVar()
	{
		m_VarDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Remove a variable from internal storage.
		\throw nothrow

		如果变量存在，则删除变量。如果变量不存在，则不执行任何操作。
	*/
	void ParserBase::RemoveVar(const string_type &a_strVarName)
	{
		varmap_type::iterator item = m_VarDef.find(a_strVarName);
		if (item != m_VarDef.end())
		{
			m_VarDef.erase(item);
			ReInit();
		}
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all functions.
		\post 重置解析器为字符串解析模式。
		\throw nothrow
	*/
	void ParserBase::ClearFun()
	{
		m_FunDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined constants.

		将数值和字符串常量从内部存储中移除。
		\post 重置解析器为字符串解析模式。
		\throw nothrow
	*/
	void ParserBase::ClearConst()
	{
		m_ConstDef.clear();
		m_StrVarDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined postfix operators.
		\post 重置解析器为字符串解析模式。
		\throw nothrow
	*/
	void ParserBase::ClearPostfixOprt()
	{
		m_PostOprtDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear all user defined binary operators.
		\post 重置解析器为字符串解析模式。
		\throw nothrow
	*/
	void ParserBase::ClearOprt()
	{
		m_OprtDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Clear the user defined Prefix operators.
		\post 重置解析器为字符串解析模式。
		\throw nothrow
	*/
	void ParserBase::ClearInfixOprt()
	{
		m_InfixOprtDef.clear();
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Enable or disable the formula optimization feature.
		\post 重置解析器为字符串解析模式。
		\throw nothrow
	*/
	void ParserBase::EnableOptimizer(bool a_bIsOn)
	{
		m_vRPN.EnableOptimizer(a_bIsOn);
		ReInit();
	}

	//---------------------------------------------------------------------------
	/** \brief Enable the dumping of bytecode and stack content on the console.
		\param bDumpCmd 启用将当前字节码转储到控制台的标志。
		\param bDumpStack 启用将堆栈内容转储到控制台的标志。

	   此函数仅用于调试！
	*/
	void ParserBase::EnableDebugDump(bool bDumpCmd, bool bDumpStack)
	{
		ParserBase::g_DbgDumpCmdCode = bDumpCmd;
		ParserBase::g_DbgDumpStack = bDumpStack;
	}

	//------------------------------------------------------------------------------
	/** \brief Enable or disable the built in binary operators.
		\throw nothrow
		\sa m_bBuiltInOp, ReInit()

	  如果禁用内置二元操作符，将不会有二元操作符被定义。因此，您必须手动逐个添加它们。不可能选择性地禁用内置操作符。此函数通过调用 ReInit() 来重新初始化解析器。
	*/
	void ParserBase::EnableBuiltInOprt(bool a_bIsOn)
	{
		m_bBuiltInOp = a_bIsOn;
		ReInit();
	}

	//------------------------------------------------------------------------------
	/** \brief Query status of built in variables.
		\return #m_bBuiltInOp；如果启用了内置操作符，则返回 true。
		\throw nothrow
	*/
	bool ParserBase::HasBuiltInOprt() const
	{
		return m_bBuiltInOp;
	}

	//------------------------------------------------------------------------------
	/** \brief Get the argument separator character.
	 */
	char_type ParserBase::GetArgSep() const
	{
		return m_pTokenReader->GetArgSep();
	}

	//------------------------------------------------------------------------------
	/** \brief Set argument separator.
		\param cArgSep 参数分隔字符。
	*/
	void ParserBase::SetArgSep(char_type cArgSep)
	{
		m_pTokenReader->SetArgSep(cArgSep);
	}
	// 该程序实现了一个解析器的基本功能。它可以解析输入的数学表达式，并进行语法检查和创建字节码。代码中的注释提供了关于每个函数的详细说明，包括函数的作用、输入参数和实现细节。这些函数包括解析表达式、处理错误、清除变量、清除函数、清除常量、清除运算符、启用优化功能、启用调试功能等。
	//------------------------------------------------------------------------------
/** \brief 打印栈的内容。

  该函数仅用于调试。
*/
void ParserBase::StackDump(const std::stack<token_type> &a_stVal, const std::stack<token_type> &a_stOprt) const
{
    std::stack<token_type> stOprt(a_stOprt);
    std::stack<token_type> stVal(a_stVal);

    mu::console() << _T("\nValue stack:\n");
    while (!stVal.empty())
    {
        token_type val = stVal.top();
        stVal.pop();

        if (val.GetType() == tpSTR)
            mu::console() << _T(" \"") << val.GetAsString() << _T("\" ");
        else
            mu::console() << _T(" ") << val.GetVal() << _T(" ");
    }
    mu::console() << "\nOperator stack:\n";

    while (!stOprt.empty())
    {
        if (stOprt.top().GetCode() <= cmASSIGN)
        {
            mu::console() << _T("OPRT_INTRNL \"")
                          << ParserBase::c_DefaultOprt[stOprt.top().GetCode()]
                          << _T("\" \n");
        }
        else
        {
            switch (stOprt.top().GetCode())
            {
            case cmVAR:
                mu::console() << _T("VAR\n");
                break;
            case cmVAL:
                mu::console() << _T("VAL\n");
                break;
            case cmFUNC:
                mu::console()
                    << _T("FUNC \"")
                    << stOprt.top().GetAsString()
                    << _T("\"\n");
                break;

            case cmFUNC_BULK:
                mu::console()
                    << _T("FUNC_BULK \"")
                    << stOprt.top().GetAsString()
                    << _T("\"\n");
                break;

            case cmOPRT_INFIX:
                mu::console() << _T("OPRT_INFIX \"")
                              << stOprt.top().GetAsString()
                              << _T("\"\n");
                break;

            case cmOPRT_BIN:
                mu::console() << _T("OPRT_BIN \"")
                              << stOprt.top().GetAsString()
                              << _T("\"\n");
                break;

            case cmFUNC_STR:
                mu::console() << _T("FUNC_STR\n");
                break;
            case cmEND:
                mu::console() << _T("END\n");
                break;
            case cmUNKNOWN:
                mu::console() << _T("UNKNOWN\n");
                break;
            case cmBO:
                mu::console() << _T("BRACKET \"(\"\n");
                break;
            case cmBC:
                mu::console() << _T("BRACKET \")\"\n");
                break;
            case cmIF:
                mu::console() << _T("IF\n");
                break;
            case cmELSE:
                mu::console() << _T("ELSE\n");
                break;
            case cmENDIF:
                mu::console() << _T("ENDIF\n");
                break;
            default:
                mu::console() << stOprt.top().GetCode() << _T(" ");
                break;
            }
        }
        stOprt.pop();
    }

    mu::console() << dec << endl;
}

/** \brief 计算结果。

  关于常量正确性的说明：
  我认为Calc是一个const函数很重要。
  由于缓存操作，Calc仅改变内部变量的状态，除了一个例外，
  即m_UsedVar在字符串解析期间被重置，并且可以从外部访问。
  Calc非const，而GetUsedVar非const，因为它明确调用Eval()强制进行此更新。

  \pre 必须设置一个公式。
  \pre 必须设置变量（如果需要）

  \sa #m_pParseFormula
  \return 评估结果
  \throw ParseException 如果没有设置公式或与公式相关的任何其他错误。
*/
value_type ParserBase::Eval() const
{
    return (this->*m_pParseFormula)();
}
// StackDump函数用于打印栈的内容，用于调试目的。
// Eval函数用于计算表达式的结果。

	//------------------------------------------------------------------------------
/** \brief 评估包含逗号分隔子表达式的表达式
    \param [out] nStackSize 可用结果的总数
    \return 指向包含所有表达式结果的数组的指针

    这个成员函数可以用于检索由多个逗号分隔子表达式组成的表达式的所有结果（例如 "x+y, sin(x), cos(y)"）。
*/
value_type *ParserBase::Eval(int &nStackSize) const
{
    if (m_vRPN.GetSize() > 0)
    {
        ParseCmdCode();
    }
    else
    {
        ParseString();
    }

    nStackSize = m_nFinalResultIdx;

    // （由于历史原因，栈从位置1开始）
    return &m_vStackBuffer[1];
}

//---------------------------------------------------------------------------
/** \brief 返回计算栈上的结果数。

    如果表达式包含逗号分隔子表达式（例如 "sin(y), x+y"），可能会有多个返回值。
    此函数返回可用结果的数量。
*/
int ParserBase::GetNumResults() const
{
    return m_nFinalResultIdx;
}

//---------------------------------------------------------------------------
void ParserBase::Eval(value_type *results, int nBulkSize)
{
    CreateRPN();

    int i = 0;

#ifdef MUP_USE_OPENMP
    // #define DEBUG_OMP_STUFF
#ifdef DEBUG_OMP_STUFF
    int *pThread = new int[nBulkSize];
    int *pIdx = new int[nBulkSize];
#endif

    int nMaxThreads = std::min(omp_get_max_threads(), s_MaxNumOpenMPThreads);
    int nThreadID = 0;

#ifdef DEBUG_OMP_STUFF
    int ct = 0;
#endif
    omp_set_num_threads(nMaxThreads);

#pragma omp parallel for schedule(static, std::max(nBulkSize / nMaxThreads, 1)) private(nThreadID)
    for (i = 0; i < nBulkSize; ++i)
    {
        nThreadID = omp_get_thread_num();
        results[i] = ParseCmdCodeBulk(i, nThreadID);

#ifdef DEBUG_OMP_STUFF
#pragma omp critical
        {
            pThread[ct] = nThreadID;
            pIdx[ct] = i;
            ct++;
        }
#endif
    }

#ifdef DEBUG_OMP_STUFF
    FILE *pFile = fopen("bulk_dbg.txt", "w");
    for (i = 0; i < nBulkSize; ++i)
    {
        fprintf(pFile, "idx: %d  thread: %d \n", pIdx[i], pThread[i]);
    }

    delete[] pIdx;
    delete[] pThread;

    fclose(pFile);
#endif

#else
    for (i = 0; i < nBulkSize; ++i)
    {
        results[i] = ParseCmdCodeBulk(i, 0);
    }
#endif
}
} // namespace mu

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
// Eval(int &nStackSize) const：评估包含逗号分隔子表达式的表达式，并返回所有结果的指针。函数内部根据表达式的类型选择相应的解析方法，并将结果存储在栈中。最后，将栈中的结果数量保存在nStackSize参数中，并返回结果数组的指针。

// GetNumResults() const：返回计算栈中的结果数。如果表达式包含逗号分隔子表达式，可能会有多个返回值。该函数返回可用结果的数量。

// Eval(value_type *results, int nBulkSize)：对给定的表达式数组进行批量评估。函数内部调用CreateRPN()创建逆波兰表达式，并通过循环对每个表达式进行解析和计算。如果启用了OpenMP多线程支持，则使用多线程并行计算，否则使用单线程计算。最后，将计算结果存储在给定的结果数组results中。

// 整个程序的功能是：提供了对包含逗号分隔子表达式的表达式进行评估和计算的功能。用户可以通过调用相应的函数来获取表达式的结果，并可以根据需要进行单个或批量的计算。