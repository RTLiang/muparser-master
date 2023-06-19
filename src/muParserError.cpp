
#include "muParserError.h"
#include <exception>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26812) // MSVC wants to force me te use enum classes or bother me with pointless warnings
#endif

namespace mu
{
	//------------------------------------------------------------------------------
	// 返回 ParserErrorMsg 的实例
	const ParserErrorMsg &ParserErrorMsg::Instance()
	{
		// 创建静态的 ParserErrorMsg 实例
		static const ParserErrorMsg instance;
		return instance;
	}

	//------------------------------------------------------------------------------
	// 重载操作符 []，用于获取指定索引位置的错误信息
	string_type ParserErrorMsg::operator[](unsigned a_iIdx) const
	{
		return (a_iIdx < m_vErrMsg.size()) ? m_vErrMsg[a_iIdx] : string_type();
	}

	//---------------------------------------------------------------------------
	ParserErrorMsg::ParserErrorMsg()
		: m_vErrMsg(0)
	{
		// 调整 m_vErrMsg 的大小为 ecCOUNT（错误类型的数量）
		m_vErrMsg.resize(ecCOUNT);

		// 各个错误类型对应的错误信息
		m_vErrMsg[ecUNASSIGNABLE_TOKEN] = _T("Unexpected token \"$TOK$\" found at position $POS$.");
		m_vErrMsg[ecINTERNAL_ERROR] = _T("Internal error");
		m_vErrMsg[ecINVALID_NAME] = _T("Invalid function-, variable- or constant name: \"$TOK$\".");
		m_vErrMsg[ecINVALID_BINOP_IDENT] = _T("Invalid binary operator identifier: \"$TOK$\".");
		m_vErrMsg[ecINVALID_INFIX_IDENT] = _T("Invalid infix operator identifier: \"$TOK$\".");
		m_vErrMsg[ecINVALID_POSTFIX_IDENT] = _T("Invalid postfix operator identifier: \"$TOK$\".");
		m_vErrMsg[ecINVALID_FUN_PTR] = _T("Invalid pointer to callback function.");
		m_vErrMsg[ecEMPTY_EXPRESSION] = _T("Expression is empty.");
		m_vErrMsg[ecINVALID_VAR_PTR] = _T("Invalid pointer to variable.");
		m_vErrMsg[ecUNEXPECTED_OPERATOR] = _T("Unexpected operator \"$TOK$\" found at position $POS$");
		m_vErrMsg[ecUNEXPECTED_EOF] = _T("Unexpected end of expression at position $POS$");
		m_vErrMsg[ecUNEXPECTED_ARG_SEP] = _T("Unexpected argument separator at position $POS$");
		m_vErrMsg[ecUNEXPECTED_PARENS] = _T("Unexpected parenthesis \"$TOK$\" at position $POS$");
		m_vErrMsg[ecUNEXPECTED_FUN] = _T("Unexpected function \"$TOK$\" at position $POS$");
		m_vErrMsg[ecUNEXPECTED_VAL] = _T("Unexpected value \"$TOK$\" found at position $POS$");
		m_vErrMsg[ecUNEXPECTED_VAR] = _T("Unexpected variable \"$TOK$\" found at position $POS$");
		m_vErrMsg[ecUNEXPECTED_ARG] = _T("Function arguments used without a function (position: $POS$)");
		m_vErrMsg[ecMISSING_PARENS] = _T("Missing parenthesis");
		m_vErrMsg[ecTOO_MANY_PARAMS] = _T("Too many parameters for function \"$TOK$\" at expression position $POS$");
		m_vErrMsg[ecTOO_FEW_PARAMS] = _T("Too few parameters for function \"$TOK$\" at expression position $POS$");
		m_vErrMsg[ecDIV_BY_ZERO] = _T("Divide by zero");
		m_vErrMsg[ecDOMAIN_ERROR] = _T("Domain error");
		m_vErrMsg[ecNAME_CONFLICT] = _T("Name conflict");
		m_vErrMsg[ecOPT_PRI] = _T("Invalid value for operator priority (must be greater or equal to zero).");
		m_vErrMsg[ecBUILTIN_OVERLOAD] = _T("user defined binary operator \"$TOK$\" conflicts with a built in operator.");
		m_vErrMsg[ecUNEXPECTED_STR] = _T("Unexpected string token found at position $POS$.");
		m_vErrMsg[ecUNTERMINATED_STRING] = _T("Unterminated string starting at position $POS$.");
		m_vErrMsg[ecSTRING_EXPECTED] = _T("String function called with a non string type of argument.");
		m_vErrMsg[ecVAL_EXPECTED] = _T("String value used where a numerical argument is expected.");
		m_vErrMsg[ecOPRT_TYPE_CONFLICT] = _T("No suitable overload for operator \"$TOK$\" at position $POS$.");
		m_vErrMsg[ecSTR_RESULT] = _T("Strings must only be used as function arguments!");
		m_vErrMsg[ecGENERIC] = _T("Parser error.");
		m_vErrMsg[ecLOCALE] = _T("Decimal separator is identic to function argument separator.");
		m_vErrMsg[ecUNEXPECTED_CONDITIONAL] = _T("The \"$TOK$\" operator must be preceded by a closing bracket.");
		m_vErrMsg[ecMISSING_ELSE_CLAUSE] = _T("If-then-else operator is missing an else clause");
		m_vErrMsg[ecMISPLACED_COLON] = _T("Misplaced colon at position $POS$");
		m_vErrMsg[ecUNREASONABLE_NUMBER_OF_COMPUTATIONS] = _T("Number of computations to small for bulk mode. (Vectorisation overhead too costly)");
		m_vErrMsg[ecIDENTIFIER_TOO_LONG] = _T("Identifier too long.");
		m_vErrMsg[ecEXPRESSION_TOO_LONG] = _T("Expression too long.");
		m_vErrMsg[ecINVALID_CHARACTERS_FOUND] = _T("Invalid non printable characters found in expression/identifer!");

		for (int i = 0; i < ecCOUNT; ++i)
		{
			// 检查错误信息是否为空
			if (!m_vErrMsg[i].length())
				throw std::runtime_error("Error definitions are incomplete!");
		}
	}
	/* 这段代码是C++中用于构造ParserErrorMsg类的构造函数。ParserErrorMsg是一个错误信息类，用于存储与解析器相关的错误信息。

	构造函数的作用是初始化m_vErrMsg向量，该向量存储了各个错误类型对应的错误信息。首先，通过调用resize(ecCOUNT) 来调整m_vErrMsg的大小，使其与错误类型的数量一致。然后，依次为各个错误类型赋值对应的错误信息。

		这段代码还包含一些错误信息的定义，例如ecUNASSIGNABLE_TOKEN对应的错误信息是Unexpected token \"$TOK$\" found at position $POS$.。

		最后，通过检查错误信息是否为空来确保错误信息的定义完整性。如果存在错误信息为空的情况，则抛出一个运行时错误。
*/
	//---------------------------------------------------------------------------
	//
	//  ParserError类
	//
	//---------------------------------------------------------------------------

	/** \brief 默认构造函数。 */
	ParserError::ParserError()
		: m_strMsg(), m_strFormula(), m_strTok(), m_iPos(-1), m_iErrc(ecUNDEFINED), m_ErrMsg(ParserErrorMsg::Instance())
	{
	}

	//------------------------------------------------------------------------------
	/** \brief 仅用于内部异常的构造函数。

	  它只包含错误代码，没有其他信息。
	*/
	ParserError::ParserError(EErrorCodes a_iErrc)
		: m_strMsg(), m_strFormula(), m_strTok(), m_iPos(-1), m_iErrc(a_iErrc), m_ErrMsg(ParserErrorMsg::Instance())
	{
		m_strMsg = m_ErrMsg[m_iErrc];
		stringstream_type stream;
		stream << (int)m_iPos;
		ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
		ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
	}

	//------------------------------------------------------------------------------
	/** \brief 用消息文本构造错误。 */
	ParserError::ParserError(const string_type &sMsg)
		: m_ErrMsg(ParserErrorMsg::Instance())
	{
		Reset();
		m_strMsg = sMsg;
	}

	//------------------------------------------------------------------------------
	/** \brief 构造一个错误对象。
		\param [in] a_iErrc 错误代码。
		\param [in] sTok 与该错误相关的标记字符串。
		\param [in] sExpr 与错误相关的表达式。
		\param [in] a_iPos 错误发生的表达式位置。
	*/
	ParserError::ParserError(EErrorCodes iErrc,
							 const string_type &sTok,
							 const string_type &sExpr,
							 int iPos)
		: m_strMsg(), m_strFormula(sExpr), m_strTok(sTok), m_iPos(iPos), m_iErrc(iErrc), m_ErrMsg(ParserErrorMsg::Instance())
	{
		m_strMsg = m_ErrMsg[m_iErrc];
		stringstream_type stream;
		stream << (int)m_iPos;
		ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
		ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
	}

	//------------------------------------------------------------------------------
	/** \brief 构造一个错误对象。
		\param [in] iErrc 错误代码。
		\param [in] iPos 错误发生的表达式位置。
		\param [in] sTok 与该错误相关的标记字符串。
	*/
	ParserError::ParserError(EErrorCodes iErrc, int iPos, const string_type &sTok)
		: m_strMsg(), m_strFormula(), m_strTok(sTok), m_iPos(iPos), m_iErrc(iErrc), m_ErrMsg(ParserErrorMsg::Instance())
	{
		m_strMsg = m_ErrMsg[m_iErrc];
		stringstream_type stream;
		stream << (int)m_iPos;
		ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
		ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
	}
	/*
	该程序实现了一个名为ParserError的类，用于表示解析器中的错误。它包含多个构造函数，用于不同的错误情况。构造函数根据传入的参数设置错误信息，并根据需要替换其中的占位符。
	*/
	//------------------------------------------------------------------------------
	/** \brief 构造一个错误对象。
	\param [in] szMsg 错误消息文本。
	\param [in] iPos 错误相关的位置。
	\param [in] sTok 与此错误相关的标记字符串。
	*/
	ParserError::ParserError(const char_type *szMsg, int iPos, const string_type &sTok)
		: m_strMsg(szMsg), m_strFormula(), m_strTok(sTok), m_iPos(iPos), m_iErrc(ecGENERIC), m_ErrMsg(ParserErrorMsg::Instance())
	{
		stringstream_type stream;
		stream << (int)m_iPos;
		ReplaceSubString(m_strMsg, _T("$POS$"), stream.str());
		ReplaceSubString(m_strMsg, _T("$TOK$"), m_strTok);
	}

	//------------------------------------------------------------------------------
	/** \brief 拷贝构造函数。 */
	ParserError::ParserError(const ParserError &a_Obj)
		: m_strMsg(a_Obj.m_strMsg), m_strFormula(a_Obj.m_strFormula), m_strTok(a_Obj.m_strTok), m_iPos(a_Obj.m_iPos), m_iErrc(a_Obj.m_iErrc), m_ErrMsg(ParserErrorMsg::Instance())
	{
	}

	//------------------------------------------------------------------------------
	/** \brief 赋值运算符。 */
	ParserError &ParserError::operator=(const ParserError &a_Obj)
	{
		if (this == &a_Obj)
			return *this;
		m_strMsg = a_Obj.m_strMsg;
		m_strFormula = a_Obj.m_strFormula;
		m_strTok = a_Obj.m_strTok;
		m_iPos = a_Obj.m_iPos;
		m_iErrc = a_Obj.m_iErrc;
		return *this;
	}

	//------------------------------------------------------------------------------
	ParserError::~ParserError()
	{
	}

	//------------------------------------------------------------------------------
	/** \brief 用另一个字符串替换源字符串中的所有出现。
	\param strFind 应该被替换的字符串。
	\param strReplaceWith 应该插入替代strFind的字符串。
	*/
	void ParserError::ReplaceSubString(string_type &strSource,
									   const string_type &strFind,
									   const string_type &strReplaceWith)
	{
		string_type strResult;
		string_type::size_type iPos(0), iNext(0);
		for (;;)
		{
			iNext = strSource.find(strFind, iPos);
			strResult.append(strSource, iPos, iNext - iPos);

			if (iNext == string_type::npos)
				break;

			strResult.append(strReplaceWith);
			iPos = iNext + strFind.length();
		}

		strSource.swap(strResult);
	}

	//------------------------------------------------------------------------------
	/** \brief 重置错误对象。 */
	void ParserError::Reset()
	{
		m_strMsg = _T("");
		m_strFormula = _T("");
		m_strTok = _T("");
		m_iPos = -1;
		m_iErrc = ecUNDEFINED;
	}

	//------------------------------------------------------------------------------
	/*
	这段C++代码定义了一个名为ParserError的类，用于表示解析器中的错误对象。代码实现了构造函数、拷贝构造函数、赋值运算符、析构函数和一些辅助函数。

	ParserError类的构造函数根据给定的错误消息文本、位置和标记字符串创建一个错误对象。拷贝构造函数和赋值运算符用于复制和赋值ParserError对象。析构函数用于清理对象。ReplaceSubString函数用于在源字符串中替换所有出现的子字符串。Reset函数用于重置错误对象的成员变量。

	这段代码实现了ParserError类的基本功能，用于创建、复制、赋值和重置错误对象。
	*/
	//------------------------------------------------------------------------------
	/** \brief 设置与此错误相关的表达式。 */
	void ParserError::SetFormula(const string_type &a_strFormula)
	{
		m_strFormula = a_strFormula;
	}

	//------------------------------------------------------------------------------
	/** \brief 获取与此错误相关的表达式。*/
	const string_type &ParserError::GetExpr() const
	{
		return m_strFormula;
	}

	//------------------------------------------------------------------------------
	/** \brief 返回此错误的消息字符串。 */
	const string_type &ParserError::GetMsg() const
	{
		return m_strMsg;
	}

	//------------------------------------------------------------------------------
	/** \brief 返回与错误相关的公式位置。

	如果错误与特定位置无关，则返回-1。
	*/
	int ParserError::GetPos() const
	{
		return m_iPos;
	}

	//------------------------------------------------------------------------------
	/** \brief 返回与此标记相关的字符串（如果有）。 */
	const string_type &ParserError::GetToken() const
	{
		return m_strTok;
	}

	//------------------------------------------------------------------------------
	/** \brief 返回错误代码。 */
	EErrorCodes ParserError::GetCode() const
	{
		return m_iErrc;
	}
} // namespace mu
/*
这段代码实现了一个名为 ParserError 的类，该类包含了处理解析器错误的相关功能。它提供了设置和获取与错误相关的表达式、消息字符串、位置、标记和错误代码的方法。
*/
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
