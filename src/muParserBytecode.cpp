/*

	 _____  __ _____________ _______  ______ ___________
	/     \|  |  \____ \__  \\_  __ \/  ___// __ \_  __ \
   |  Y Y  \  |  /  |_> > __ \|  | \/\___ \\  ___/|  | \/
   |__|_|  /____/|   __(____  /__|  /____  >\___  >__|
		 \/      |__|       \/           \/     \/
   Copyright (C) 2004 - 2022 Ingo Berg

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
#include "muParserBytecode.h"

#include <algorithm>
#include <string>
#include <stack>
#include <vector>
#include <iostream>

#include "muParserDef.h"
#include "muParserError.h"
#include "muParserToken.h"
#include "muParserTemplateMagic.h"

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26812)
#endif

namespace mu
{
	/** \brief 字节码的默认构造函数。 */
	ParserByteCode::ParserByteCode()
		: m_iStackPos(0), m_iMaxStackSize(0), m_vRPN(), m_bEnableOptimizer(true)
	{
		m_vRPN.reserve(50);
	}

	/** \brief 复制构造函数。
		实现为Assign(const ParserByteCode &a_ByteCode)。
	*/
	ParserByteCode::ParserByteCode(const ParserByteCode &a_ByteCode)
	{
		Assign(a_ByteCode);
	}

	/** \brief 赋值运算符。
		实现为Assign(const ParserByteCode &a_ByteCode)。
	*/
	ParserByteCode &ParserByteCode::operator=(const ParserByteCode &a_ByteCode)
	{
		Assign(a_ByteCode);
		return *this;
	}

	void ParserByteCode::EnableOptimizer(bool bStat)
	{
		m_bEnableOptimizer = bStat;
	}

	/** \brief 将另一个对象的状态复制到此对象。
		\throw nowthrow
	*/
	void ParserByteCode::Assign(const ParserByteCode &a_ByteCode)
	{
		if (this == &a_ByteCode)
			return;

		m_iStackPos = a_ByteCode.m_iStackPos;
		m_vRPN = a_ByteCode.m_vRPN;
		m_iMaxStackSize = a_ByteCode.m_iMaxStackSize;
		m_bEnableOptimizer = a_ByteCode.m_bEnableOptimizer;
	}

	/** \brief 向字节码添加变量指针。
		\param a_pVar 要添加的指针。
		\throw nothrow
	*/
	void ParserByteCode::AddVar(value_type *a_pVar)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// 优化不适用
		SToken tok;
		tok.Cmd = cmVAR;
		tok.Val.ptr = a_pVar;
		tok.Val.data = 1;
		tok.Val.data2 = 0;
		m_vRPN.push_back(tok);
	}

	/** \brief 向字节码添加值。
		字节码中的值条目包括：
		<ul>
		  <li>值在值数组中的位置</li>
		  <li>操作符代码，根据ParserToken::cmVAL</li>
		  <li>存储在#mc_iSizeVal个字节码条目中的值。</li>
		</ul>
		\param a_pVal 要添加的值。
		\throw nothrow
	*/
	void ParserByteCode::AddVal(value_type a_fVal)
	{
		++m_iStackPos;
		m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

		// 如果优化不适用
		SToken tok;
		tok.Cmd = cmVAL;
		tok.Val.ptr = nullptr;
		tok.Val.data = 0;
		tok.Val.data2 = a_fVal;
		m_vRPN.push_back(tok);
	}

	void ParserByteCode::ConstantFolding(ECmdCode a_Oprt)
	{
		std::size_t sz = m_vRPN.size();			  // 获取逆波兰表达式的长度
		value_type &x = m_vRPN[sz - 2].Val.data2; // 获取倒数第二个操作数
		value_type &y = m_vRPN[sz - 1].Val.data2; // 获取最后一个操作数

		switch (a_Oprt) // 根据操作符进行计算
		{
		case cmLAND:
			x = (int)x && (int)y;
			m_vRPN.pop_back();
			break; // 逻辑与操作
		case cmLOR:
			x = (int)x || (int)y;
			m_vRPN.pop_back();
			break; // 逻辑或操作
		case cmLT:
			x = x < y;
			m_vRPN.pop_back();
			break; // 小于操作
		case cmGT:
			x = x > y;
			m_vRPN.pop_back();
			break; // 大于操作
		case cmLE:
			x = x <= y;
			m_vRPN.pop_back();
			break; // 小于等于操作
		case cmGE:
			x = x >= y;
			m_vRPN.pop_back();
			break; // 大于等于操作
		case cmNEQ:
			x = x != y;
			m_vRPN.pop_back();
			break; // 不等于操作
		case cmEQ:
			x = x == y;
			m_vRPN.pop_back();
			break; // 等于操作
		case cmADD:
			x = x + y;
			m_vRPN.pop_back();
			break; // 加法操作
		case cmSUB:
			x = x - y;
			m_vRPN.pop_back();
			break; // 减法操作
		case cmMUL:
			x = x * y;
			m_vRPN.pop_back();
			break;	// 乘法操作
		case cmDIV: // 除法操作
			x = x / y;
			m_vRPN.pop_back();
			break;

		case cmPOW: // 幂运算
			x = MathImpl<value_type>::Pow(x, y);
			m_vRPN.pop_back();
			break;

		default:
			break;
		} // switch opcode
	}
	// 此代码用于执行常量折叠（Constant Folding）操作。根据传入的操作符（a_Oprt），对逆波兰表达式（m_vRPN）中的操作数进行相应的计算。根据操作符的不同，可以进行逻辑与、逻辑或、小于、大于、小于等于、大于等于、不等于、等于、加法、减法、乘法、除法和幂运算等操作。每次计算完成后，将计算结果存储在倒数第二个操作数的位置，并将最后一个操作数从逆波兰表达式中移除。

	// 功能： 执行常量折叠操作，根据给定的操作符对逆波兰表达式中的操作数进行计算，并更新表达式中的值。

	// 注意： 以上代码中使用的变量和类型可能未在提供的代码片段中定义，因此无法确定其具体含义和数据类型。
	void ParserByteCode::AddOp(ECmdCode a_Oprt)
	{
		bool bOptimized = false;

		if (m_bEnableOptimizer)
		{
			std::size_t sz = m_vRPN.size();

			// 检查可折叠的常量，例如：cmVAL cmVAL cmADD
			// 其中 cmADD 可以代表应用于两个常量值的任何二元运算符。
			if (sz >= 2 && m_vRPN[sz - 2].Cmd == cmVAL && m_vRPN[sz - 1].Cmd == cmVAL)
			{
				ConstantFolding(a_Oprt);
				bOptimized = true;
			}
			else
			{
				switch (a_Oprt)
				{
				case cmPOW:
					// 低阶多项式的优化
					if (m_vRPN[sz - 2].Cmd == cmVAR && m_vRPN[sz - 1].Cmd == cmVAL)
					{
						if (m_vRPN[sz - 1].Val.data2 == 0)
						{
							m_vRPN[sz - 2].Cmd = cmVAL;
							m_vRPN[sz - 2].Val.ptr = nullptr;
							m_vRPN[sz - 2].Val.data = 0;
							m_vRPN[sz - 2].Val.data2 = 1;
						}
						else if (m_vRPN[sz - 1].Val.data2 == 1)
							m_vRPN[sz - 2].Cmd = cmVAR;
						else if (m_vRPN[sz - 1].Val.data2 == 2)
							m_vRPN[sz - 2].Cmd = cmVARPOW2;
						else if (m_vRPN[sz - 1].Val.data2 == 3)
							m_vRPN[sz - 2].Cmd = cmVARPOW3;
						else if (m_vRPN[sz - 1].Val.data2 == 4)
							m_vRPN[sz - 2].Cmd = cmVARPOW4;
						else
							break;

						m_vRPN.pop_back();
						bOptimized = true;
					}
					break;

				case cmSUB:
				case cmADD:
					// 基于模式识别的简单优化，针对大量不同的加法/减法字节码组合
					if ((m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVAL) ||
						(m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVAR) ||
						(m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVARMUL) ||
						(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVAL) ||
						(m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVAR && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) ||
						(m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVARMUL && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) ||
						(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVAR && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr) ||
						(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVARMUL && m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr))
					{
						MUP_ASSERT(
							(m_vRPN[sz - 2].Val.ptr == nullptr && m_vRPN[sz - 1].Val.ptr != nullptr) ||
							(m_vRPN[sz - 2].Val.ptr != nullptr && m_vRPN[sz - 1].Val.ptr == nullptr) ||
							(m_vRPN[sz - 2].Val.ptr == m_vRPN[sz - 1].Val.ptr));

						m_vRPN[sz - 2].Cmd = cmVARMUL;
						m_vRPN[sz - 2].Val.ptr = (value_type *)((long long)(m_vRPN[sz - 2].Val.ptr) | (long long)(m_vRPN[sz - 1].Val.ptr)); // 变量
						m_vRPN[sz - 2].Val.data2 += ((a_Oprt == cmSUB) ? -1 : 1) * m_vRPN[sz - 1].Val.data2;								// 偏移量
						m_vRPN[sz - 2].Val.data += ((a_Oprt == cmSUB) ? -1 : 1) * m_vRPN[sz - 1].Val.data;									// 乘法因子
						m_vRPN.pop_back();
						bOptimized = true;
					}
					break;

				case cmMUL:
					if ((m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVAL) ||
						(m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVAR))
					{
						m_vRPN[sz - 2].Cmd = cmVARMUL;
						m_vRPN[sz - 2].Val.ptr = (value_type *)((long long)(m_vRPN[sz - 2].Val.ptr) | (long long)(m_vRPN[sz - 1].Val.ptr));
						m_vRPN[sz - 2].Val.data = m_vRPN[sz - 2].Val.data2 + m_vRPN[sz - 1].Val.data2;
						m_vRPN[sz - 2].Val.data2 = 0;
						m_vRPN.pop_back();
						bOptimized = true;
					}
					else if (
						(m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVARMUL) ||
						(m_vRPN[sz - 1].Cmd == cmVARMUL && m_vRPN[sz - 2].Cmd == cmVAL))
					{
						// 优化：2*(3*b+1) 或者 (3*b+1)*2 -> 6*b+2
						m_vRPN[sz - 2].Cmd = cmVARMUL;
						m_vRPN[sz - 2].Val.ptr = (value_type *)((long long)(m_vRPN[sz - 2].Val.ptr) | (long long)(m_vRPN[sz - 1].Val.ptr));
						if (m_vRPN[sz - 1].Cmd == cmVAL)
						{
							m_vRPN[sz - 2].Val.data *= m_vRPN[sz - 1].Val.data2;
							m_vRPN[sz - 2].Val.data2 *= m_vRPN[sz - 1].Val.data2;
						}
						else
						{
							m_vRPN[sz - 2].Val.data = m_vRPN[sz - 1].Val.data * m_vRPN[sz - 2].Val.data2;
							m_vRPN[sz - 2].Val.data2 = m_vRPN[sz - 1].Val.data2 * m_vRPN[sz - 2].Val.data2;
						}
						m_vRPN.pop_back();
						bOptimized = true;
					}
					else if (
						m_vRPN[sz - 1].Cmd == cmVAR && m_vRPN[sz - 2].Cmd == cmVAR &&
						m_vRPN[sz - 1].Val.ptr == m_vRPN[sz - 2].Val.ptr)
					{
						// 优化：a*a -> a^2
						m_vRPN[sz - 2].Cmd = cmVARPOW2;
						m_vRPN.pop_back();
						bOptimized = true;
					}
					break;

				case cmDIV:
					if (m_vRPN[sz - 1].Cmd == cmVAL && m_vRPN[sz - 2].Cmd == cmVARMUL && m_vRPN[sz - 1].Val.data2 != 0)
					{
						// 优化：4*a/2 -> 2*a
						m_vRPN[sz - 2].Val.data /= m_vRPN[sz - 1].Val.data2;
						m_vRPN[sz - 2].Val.data2 /= m_vRPN[sz - 1].Val.data2;
						m_vRPN.pop_back();
						bOptimized = true;
					}
					break;

					// 其他操作码不进行优化
				default:
					break;
				} // switch a_Oprt
			}
		}

		// 通过以上代码可以实现字节码解析器的优化功能。

		// 如果无法应用优化，直接写入数值
		if (!bOptimized)
		{
			--m_iStackPos;
			SToken tok;
			tok.Cmd = a_Oprt;
			m_vRPN.push_back(tok);
		}
		// 如果无法应用优化，将数值写入RPN向量。

		void ParserByteCode::AddIfElse(ECmdCode a_Oprt)
		{
			SToken tok;
			tok.Cmd = a_Oprt;
			m_vRPN.push_back(tok);
		}
		// 向RPN向量中添加条件判断指令。

		/** \brief 添加赋值操作符

		字节码中的操作符条目包括：
		<ul>
		  <li>cmASSIGN代码</li>
		  <li>目标变量的指针</li>
		</ul>

		\sa  ParserToken::ECmdCode
		*/
		void ParserByteCode::AddAssignOp(value_type * a_pVar)
		{
			--m_iStackPos;

			SToken tok;
			tok.Cmd = cmASSIGN;
			tok.Oprt.ptr = a_pVar;
			m_vRPN.push_back(tok);
		}
		// 向RPN向量中添加赋值操作符。注释解释了字节码中操作符的条目内容，包括操作符代码和目标变量的指针。
		/** \brief 添加函数到字节码中。

		\param a_iArgc 参数数量，负数表示多参数函数。
		\param a_pFun 函数回调的指针。
		*/
		void ParserByteCode::AddFun(generic_callable_type a_pFun, int a_iArgc, bool isFunctionOptimizable)
		{
			std::size_t sz = m_vRPN.size();
			bool optimize = false;

			// 只对具有固定数量大于一个参数的函数进行优化
			if (isFunctionOptimizable && m_bEnableOptimizer && a_iArgc > 0)
			{
				// <ibg 2020-06-10/> 一元加是无操作
				if (a_pFun == generic_callable_type{(erased_fun_type)&MathImpl<value_type>::UnaryPlus, nullptr})
					return;

				optimize = true;

				for (int i = 0; i < std::abs(a_iArgc); ++i)
				{
					if (m_vRPN[sz - i - 1].Cmd != cmVAL)
					{
						optimize = false;
						break;
					}
				}
			}

			if (optimize)
			{
				value_type val = 0;
				switch (a_iArgc)
				{
				case 1:
					val = a_pFun.call_fun<1>(m_vRPN[sz - 1].Val.data2);
					break;
				case 2:
					val = a_pFun.call_fun<2>(m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 3:
					val = a_pFun.call_fun<3>(m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 4:
					val = a_pFun.call_fun<4>(m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 5:
					val = a_pFun.call_fun<5>(m_vRPN[sz - 5].Val.data2, m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 6:
					val = a_pFun.call_fun<6>(m_vRPN[sz - 6].Val.data2, m_vRPN[sz - 5].Val.data2, m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 7:
					val = a_pFun.call_fun<7>(m_vRPN[sz - 7].Val.data2, m_vRPN[sz - 6].Val.data2, m_vRPN[sz - 5].Val.data2, m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 8:
					val = a_pFun.call_fun<8>(m_vRPN[sz - 8].Val.data2, m_vRPN[sz - 7].Val.data2, m_vRPN[sz - 6].Val.data2, m_vRPN[sz - 5].Val.data2, m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 9:
					val = a_pFun.call_fun<9>(m_vRPN[sz - 9].Val.data2, m_vRPN[sz - 8].Val.data2, m_vRPN[sz - 7].Val.data2, m_vRPN[sz - 6].Val.data2, m_vRPN[sz - 5].Val.data2, m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				case 10:
					val = a_pFun.call_fun<10>(m_vRPN[sz - 10].Val.data2, m_vRPN[sz - 9].Val.data2, m_vRPN[sz - 8].Val.data2, m_vRPN[sz - 7].Val.data2, m_vRPN[sz - 6].Val.data2, m_vRPN[sz - 5].Val.data2, m_vRPN[sz - 4].Val.data2, m_vRPN[sz - 3].Val.data2, m_vRPN[sz - 2].Val.data2, m_vRPN[sz - 1].Val.data2);
					break;
				default:
					// For now functions with unlimited number of arguments are not optimized
					throw ParserError(ecINTERNAL_ERROR);
				}

				// 移除折叠的值
				m_vRPN.erase(m_vRPN.end() - a_iArgc, m_vRPN.end());

				SToken tok;
				tok.Cmd = cmVAL;
				tok.Val.data = 0;
				tok.Val.data2 = val;
				tok.Val.ptr = nullptr;
				m_vRPN.push_back(tok);
			}
			else
			{
				SToken tok;
				tok.Cmd = cmFUNC;
				tok.Fun.argc = a_iArgc;
				tok.Fun.cb = a_pFun;
				m_vRPN.push_back(tok);
			}

			m_iStackPos = m_iStackPos - std::abs(a_iArgc) + 1;
			m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);
		}

		/** \brief 向字节码中添加批量函数。

			\param a_iArgc 参数个数，负数表示多参数函数。
			\param a_pFun 函数回调指针。
		*/
		void ParserByteCode::AddBulkFun(generic_callable_type a_pFun, int a_iArgc)
		{
			m_iStackPos = m_iStackPos - a_iArgc + 1;
			m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);

			SToken tok;
			tok.Cmd = cmFUNC_BULK;
			tok.Fun.argc = a_iArgc;
			tok.Fun.cb = a_pFun;
			m_vRPN.push_back(tok);
		}

		/** \brief 向解析器字节码中添加字符串函数入口。
			\throw nothrow

			字符串函数入口由返回值的堆栈位置组成，后跟一个cmSTRFUNC代码、函数指针和解析器维护的字符串缓冲区中的索引。
		*/
		void ParserByteCode::AddStrFun(generic_callable_type a_pFun, int a_iArgc, int a_iIdx)
		{
			m_iStackPos = m_iStackPos - a_iArgc + 1;

			SToken tok;
			tok.Cmd = cmFUNC_STR;
			tok.Fun.argc = a_iArgc;
			tok.Fun.idx = a_iIdx;
			tok.Fun.cb = a_pFun;
			m_vRPN.push_back(tok);

			m_iMaxStackSize = std::max(m_iMaxStackSize, (size_t)m_iStackPos);
		}

		/** \brief 向字节码添加结束标记。

			\throw nothrow
		*/
		void ParserByteCode::Finalize()
		{
			SToken tok;
			tok.Cmd = cmEND;
			m_vRPN.push_back(tok);
			rpn_type(m_vRPN).swap(m_vRPN); // 收缩字节码向量以适应

			// 确定if-then-else跳转偏移量
			std::stack<int> stIf, stElse;
			int idx;
			for (int i = 0; i < (int)m_vRPN.size(); ++i)
			{
				switch (m_vRPN[i].Cmd)
				{
				case cmIF:
					stIf.push(i);
					break;

				case cmELSE:
					stElse.push(i);
					idx = stIf.top();
					stIf.pop();
					m_vRPN[idx].Oprt.offset = i - idx;
					break;

				case cmENDIF:
					idx = stElse.top();
					stElse.pop();
					m_vRPN[idx].Oprt.offset = i - idx;
					break;

				default:
					break;
				}
			}
		}
		// AddBulkFun函数用于向字节码中添加批量函数，参数包括函数回调指针和参数个数。
		// AddStrFun函数用于向字节码中添加字符串函数入口，参数包括函数回调指针、参数个数和字符串缓冲区中的索引。
		// Finalize函数用于向字节码添加结束标记，并进行字节码向量的收缩操作。在收缩过程中，该函数还确定了if-then-else语句的跳转偏移量。
		std::size_t ParserByteCode::GetMaxStackSize() const
		{
			return m_iMaxStackSize + 1;
		}

		/** \brief 删除字节码。

	\throw nothrow

	此函数的名称违反了我的编码准则，但这样更符合STL函数的命名规范，因此更加直观。
*/
		void ParserByteCode::clear()
		{
			m_vRPN.clear();
			m_iStackPos = 0;
			m_iMaxStackSize = 0;
		}

		/** \brief 转储字节码（仅用于调试！）。 */
		void ParserByteCode::AsciiDump()
		{
			if (!m_vRPN.size())
			{
				mu::console() << _T("无可用字节码\n");
				return;
			}

			mu::console() << _T("RPN令牌数：") << (int)m_vRPN.size() << _T("\n");
			for (std::size_t i = 0; i < m_vRPN.size() && m_vRPN[i].Cmd != cmEND; ++i)
			{
				mu::console() << std::dec << i << _T(" : \t");
				switch (m_vRPN[i].Cmd)
				{
				case cmVAL:
					mu::console() << _T("VAL \t");
					mu::console() << _T("[") << m_vRPN[i].Val.data2 << _T("]\n");
					break;

				case cmVAR:
					mu::console() << _T("VAR \t");
					mu::console() << _T("[ADDR: 0x") << std::hex << m_vRPN[i].Val.ptr << _T("]\n");
					break;

				case cmVARPOW2:
					mu::console() << _T("VARPOW2 \t");
					mu::console() << _T("[ADDR: 0x") << std::hex << m_vRPN[i].Val.ptr << _T("]\n");
					break;

				case cmVARPOW3:
					mu::console() << _T("VARPOW3 \t");
					mu::console() << _T("[ADDR: 0x") << std::hex << m_vRPN[i].Val.ptr << _T("]\n");
					break;

				case cmVARPOW4:
					mu::console() << _T("VARPOW4 \t");
					mu::console() << _T("[ADDR: 0x") << std::hex << m_vRPN[i].Val.ptr << _T("]\n");
					break;

				case cmVARMUL:
					mu::console() << _T("VARMUL \t");
					mu::console() << _T("[ADDR: 0x") << std::hex << m_vRPN[i].Val.ptr << _T("]");
					mu::console() << _T(" * [") << m_vRPN[i].Val.data << _T("]");
					mu::console() << _T(" + [") << m_vRPN[i].Val.data2 << _T("]\n");
					break;

				case cmFUNC:
					mu::console() << _T("CALL\t");
					mu::console() << _T("[ARG:") << std::dec << m_vRPN[i].Fun.argc << _T("]");
					mu::console() << _T("[ADDR: 0x") << std::hex << reinterpret_cast<void *>(m_vRPN[i].Fun.cb._pRawFun) << _T("]");
					mu::console() << _T("[USERDATA: 0x") << std::hex << reinterpret_cast<void *>(m_vRPN[i].Fun.cb._pUserData) << _T("]");
					mu::console() << _T("\n");
					break;

				case cmFUNC_STR:
					mu::console() << _T("CALL STRFUNC\t");
					mu::console() << _T("[ARG:") << std::dec << m_vRPN[i].Fun.argc << _T("]");
					mu::console() << _T("[IDX:") << std::dec << m_vRPN[i].Fun.idx << _T("]");
					mu::console() << _T("[ADDR: 0x") << std::hex << reinterpret_cast<void *>(m_vRPN[i].Fun.cb._pRawFun) << _T("]");
					mu::console() << _T("[USERDATA: 0x") << std::hex << reinterpret_cast<void *>(m_vRPN[i].Fun.cb._pUserData) << _T("]");
					mu::console() << _T("\n");
					break;

				case cmLT:
					mu::console() << _T("LT\n");
					break;
				case cmGT:
					mu::console() << _T("GT\n");
					break;
				case cmLE:
					mu::console() << _T("LE\n");
					break;
				case cmGE:
					mu::console() << _T("GE\n");
					break;
				case cmEQ:
					mu::console() << _T("EQ\n");
					break;
				case cmNEQ:
					mu::console() << _T("NEQ\n");
					break;
				case cmADD:
					mu::console() << _T("ADD\n");
					break;
				case cmLAND:
					mu::console() << _T("&&\n");
					break;
				case cmLOR:
					mu::console() << _T("||\n");
					break;
				case cmSUB:
					mu::console() << _T("SUB\n");
					break;
				case cmMUL:
					mu::console() << _T("MUL\n");
					break;
				case cmDIV:
					mu::console() << _T("DIV\n");
					break;
				case cmPOW:
					mu::console() << _T("POW\n");
					break;

				case cmIF:
					mu::console() << _T("IF\t");
					mu::console() << _T("[OFFSET:") << std::dec << m_vRPN[i].Oprt.offset << _T("]\n");
					break;

				case cmELSE:
					mu::console() << _T("ELSE\t");
					mu::console() << _T("[OFFSET:") << std::dec << m_vRPN[i].Oprt.offset << _T("]\n");
					break;

				case cmENDIF:
					mu::console() << _T("ENDIF\n");
					break;

				case cmASSIGN:
					mu::console() << _T("ASSIGN\t");
					mu::console() << _T("[ADDR: 0x") << m_vRPN[i].Oprt.ptr << _T("]\n");
					break;

				default:
					mu::console() << _T("(未知代码：") << m_vRPN[i].Cmd << _T(")\n");
					break;
				} // switch cmdCode
			}	  // while bytecode

			mu::console() << _T("END") << std::endl;
		}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
