# pragma once
# include <Siv3D.hpp>

namespace s3d
{
	/// @brief 数値パッド
	class Numpad
	{
	public:

		struct Style
		{
			/// @brief キーのサイズ（ピクセル）
			SizeF keySize{ 70, 64 };

			/// @brief キーの余白サイズ（ピクセル）
			SizeF keyMargin{ 10, 10 };

			/// @brief キーの角丸の半径（ピクセル）
			double roundRadius = 12.0;

			/// @brief キーの色
			ColorF keyColor{ 0.98 };

			/// @brief キーのホバー時の色
			ColorF keyHoveredColor{ 0.94 };

			/// @brief テキストのフォント
			Font textFont = SimpleGUI::GetFont();

			/// @brief テキストのフォントスケール
			double fontScale = 1.0;

			/// @brief テキストの色
			ColorF textColor{ 0.11 };

			/// @brief 最大入力桁数
			int32 maxDigits = 13;

			/// @brief 入力時に追記モードにするか
			bool append = false;
		};

		/// @brief キーボード入力を許可する
		static constexpr bool AllowKeyInput = true;

		/// @brief キーボード入力を許可しない
		static constexpr bool DenyKeyInput = false;

		SIV3D_NODISCARD_CXX20
		Numpad() = default;

		/// @brief 数値パッドのスタイルを指定して初期化します。
		/// @param style 数値パッドのスタイル
		SIV3D_NODISCARD_CXX20
		explicit Numpad(const Style& style) noexcept;

		/// @brief 数値パッドを更新します。
		/// @param allowKeyInput キーボードによる入力を許可する場合 `Numapd::AllowKeyInput`, それ以外の場合は `Numpad::DenyKeyInput`
		/// @return E のキーが押された場合 true, それ以外の場合は false
		[[nodiscard]]
		bool update(bool allowKeyInput = AllowKeyInput);

		/// @brief 数値パッドを描画します。
		void draw() const;

		/// @brief 数値パッドを上書きモードで開きます。
		/// @param pos 数値パッドの位置
		void open(const Vec2& pos) noexcept;

		/// @brief 数値パッドを上書きモードで開きます。
		/// @param pos 数値パッドの位置
		/// @param value 初期値
		void open(const Vec2& pos, double value) noexcept;

		/// @brief 数値パッドを閉じます。
		void close() noexcept;

		/// @brief 数値パッドが開いているかを返します。
		/// @return 数値パッドが開いている場合 true, それ以外の場合は false
		[[nodiscard]]
		bool isOpen() const noexcept;

		/// @brief 数値パッドの位置を返します。
		/// @return 数値パッドの位置
		[[nodiscard]]
		const Vec2& getPos() const noexcept;

		/// @brief 数値パッドの位置を変更します。
		/// @param pos 数値パッドの位置
		void setPos(const Vec2& pos) noexcept;

		/// @brief 数値パッドの領域を返します。
		/// @return 数値パッドの領域
		[[nodiscard]]
		RectF region() const noexcept;

		/// @brief 数値パッドのスタイルを返します。
		/// @return 数値パッドのスタイル
		[[nodiscard]]
		const Style& getStyle() const noexcept;

		/// @brief 数値パッドの最大入力桁数を返します。
		/// @return 数値パッドの最大入力桁数
		[[nodiscard]]
		int32 getMaxDigits() const noexcept;

		/// @brief 数値パッドのスタイルを変更します。
		/// @param style 数値パッドのスタイル
		void setStyle(const Style& style) noexcept;

		/// @brief 数値パッドの値を変更します。
		/// @param value 値
		void setValue(double value) noexcept;

		/// @brief 数値パッドの値を消去します。
		void clear() noexcept;

		/// @brief 数値パッドの値を文字列で返します。
		/// @return 数値パッドの値
		[[nodiscard]]
		String getText() const noexcept;

		/// @brief 数値パッドの値を桁区切り記号付きの文字列で返します。
		/// @return 数値パッドの値
		[[nodiscard]]
		String withThousandsSeparators() const noexcept;

		/// @brief 数値パッドの値を浮動小数点数として返します。
		/// @return 数値パッドの値
		[[nodiscard]]
		double getFloat() const noexcept;

		/// @brief 数値パッドの値を整数として返します。
		/// @return 数値パッドの値
		[[nodiscard]]
		int64 getInt() const noexcept;

		/// @brief 与えられた数値を、現在の数値パッドのスタイルでフォーマットします。
		/// @param value 数値
		/// @return フォーマットされた文字列
		[[nodiscard]]
		String formatValue(double value) const noexcept;

	private:

		static constexpr StringView Labels = U"789\U000F0B5C456C123E\U000F14C90.";

		Style m_style;

		Vec2 m_pos{ 0, 0 };

		String m_buffer;

		bool m_isOpen = false;

		bool m_overwrite = false;

		[[nodiscard]]
		static RoundRect GetKeyRoundRect(const Vec2& topLeft, int32 i, const Style& style)
		{
			const int32 ix = (i % 4);
			const int32 iy = (i / 4);
			const double keyStepX = (style.keySize.x + style.keyMargin.x);
			const double keyStepY = (style.keySize.y + style.keyMargin.y);
			const Vec2 keyPos = topLeft.movedBy((keyStepX * ix), (keyStepY * iy));

			if (i != 11)
			{
				return RoundRect{ keyPos, style.keySize, style.roundRadius };
			}
			else
			{
				const Vec2 rightBottom = keyPos.movedBy(style.keySize.x, (style.keySize.y * 2 + style.keyMargin.y));
				return RectF::FromPoints(keyPos, rightBottom).rounded(style.roundRadius);
			}
		}

		[[nodiscard]]
		Vec2 getTopLeft() const noexcept
		{
			return (m_pos + m_style.keyMargin);
		}

		void pushDigit(const char32 ch) noexcept
		{
			if (m_overwrite)
			{
				m_buffer.clear();
				m_overwrite = false;
			}

			if ((m_buffer == U"0") || (m_buffer == U"-0"))
			{
				m_buffer.pop_back();
			}

			m_buffer.push_back(ch);
		}

		[[nodiscard]]
		String getText(bool useThousandsSeparator) const noexcept
		{
			if (m_buffer.isEmpty())
			{
				return U"0";
			}
			else if (m_buffer == U"-")
			{
				return U"-";
			}
			else if (auto it = m_buffer.indexOf(U'.'); it == String::npos)
			{
				if (m_buffer == U"-0")
				{
					return U"-0";
				}

				if (useThousandsSeparator)
				{
					return ThousandsSeparate(ParseOr<int64>(m_buffer, 0));
				}
				else
				{
					return m_buffer;
				}
			}
			else
			{
				const int32 decimalCount = static_cast<int32>(m_buffer.size() - it - 1);

				const bool dotAtEnd = (decimalCount == 0);

				const bool isNegative = m_buffer.starts_with(U'-');

				String value = m_buffer;

				if (dotAtEnd)
				{
					value.pop_back();
				}

				String result;

				if (useThousandsSeparator)
				{
					result = ThousandsSeparate(ParseOr<double>(value, 0.0), decimalCount, Fixed::Yes);
				}
				else
				{
					result = value;
				}

				if (dotAtEnd)
				{
					result.push_back(U'.');
				}

				if (isNegative && (not result.starts_with(U'-')))
				{
					result.push_front(U'-');
				}

				return result;
			}
		}
	};

	inline Numpad::Numpad(const Style& style) noexcept
		: m_style{ style } {}

	inline bool Numpad::update(const bool allowKeyInput)
	{
		if (not m_isOpen)
		{
			return false;
		}

		const Vec2 topLeft = getTopLeft();
		int32 keyIndex = -1;

		for (int32 i = 0; i < 15; ++i)
		{
			const RoundRect key = GetKeyRoundRect(topLeft, i, m_style);

			if (key.leftClicked())
			{
				keyIndex = i;
				break;
			}
		}

		if ((keyIndex == 0) || (allowKeyInput && (Key7 | KeyNum7).down())) // [7]
		{
			pushDigit(U'7');
		}
		else if ((keyIndex == 1) || (allowKeyInput && (Key8 | KeyNum8).down())) // [8]
		{
			pushDigit(U'8');
		}
		else if ((keyIndex == 2) || (allowKeyInput && (Key9 | KeyNum9).down())) // [9]
		{
			pushDigit(U'9');
		}
		else if ((keyIndex == 3) || (allowKeyInput && (KeyBackspace | KeyDelete).down())) // [Backspace]
		{
			if (m_overwrite)
			{
				m_buffer.clear();
				m_overwrite = false;
			}
			else if (m_buffer)
			{
				m_buffer.pop_back();
			}
		}
		else if ((keyIndex == 4) || (allowKeyInput && (Key4 | KeyNum4).down())) // [4]
		{
			pushDigit(U'4');
		}
		else if ((keyIndex == 5) || (allowKeyInput && (Key5 | KeyNum5).down())) // [5]
		{
			pushDigit(U'5');
		}
		else if ((keyIndex == 6) || (allowKeyInput && (Key6 | KeyNum6).down())) // [6]
		{
			pushDigit(U'6');
		}
		else if ((keyIndex == 7) || (allowKeyInput && (KeyC | KeyClear).down())) // [C]
		{
			m_buffer.clear();
			m_overwrite = false;
		}
		else if ((keyIndex == 8) || (allowKeyInput && (Key1 | KeyNum1).down())) // [1]
		{
			pushDigit(U'1');
		}
		else if ((keyIndex == 9) || (allowKeyInput && (Key2 | KeyNum2).down())) // [2]
		{
			pushDigit(U'2');
		}
		else if ((keyIndex == 10) || (allowKeyInput && (Key3 | KeyNum3).down())) // [3]
		{
			pushDigit(U'3');
		}
		else if ((keyIndex == 11) || (allowKeyInput && (KeyE | KeyEnter | KeyNumEnter).down())) // [E]
		{
			return true;
		}
		else if ((keyIndex == 12) || (allowKeyInput && (KeyMinus | KeyNumSubtract).down())) // [±]
		{
			if (m_overwrite)
			{
				m_buffer.assign(U"-");
				m_overwrite = false;
			}
			else if (m_buffer.starts_with(U'-'))
			{
				m_buffer.pop_front();
			}
			else
			{
				m_buffer.push_front(U'-');
			}
		}
		else if ((keyIndex == 13) || (allowKeyInput && (Key0 | KeyNum0).down())) // [0]
		{
			pushDigit(U'0');
		}
		else if ((keyIndex == 14) || (allowKeyInput && (KeyPeriod | KeyNumDecimal).down())) // [.]
		{
			if (m_overwrite)
			{
				m_buffer.assign(U"0.");
				m_overwrite = false;
			}
			else if (m_buffer.isEmpty())
			{
				m_buffer.append(U"0.");
			}
			else if (m_buffer == U"-")
			{
				m_buffer.assign(U"-0.");
			}
			else if (not m_buffer.contains(U'.'))
			{
				m_buffer.push_back(U'.');
			}
		}

		if (m_style.maxDigits < m_buffer.count_if(IsDigit))
		{
			m_buffer.pop_back();
		}

		return false;
	}

	inline void Numpad::draw() const
	{
		if (not m_isOpen)
		{
			return;
		}

		const Vec2 topLeft = getTopLeft();
		const Font& font = m_style.textFont;
		const double fontSize = (m_style.keySize.minComponent() * 0.5 * m_style.fontScale);

		// キーの影
		for (int32 i = 0; i < 15; ++i)
		{
			GetKeyRoundRect(topLeft, i, m_style).drawShadow(Vec2{ 0, 1 }, 3, true);
		}

		// キー
		for (int32 i = 0; i < 15; ++i)
		{
			const RoundRect key = GetKeyRoundRect(topLeft, i, m_style);

			const bool mouseOver = key.mouseOver();

			key.draw(mouseOver ? m_style.keyHoveredColor : m_style.keyColor);

			if (mouseOver)
			{
				Cursor::RequestStyle(CursorStyle::Hand);
			}
		}

		// 数字
		for (int32 i = 0; i < 15; ++i)
		{
			const RoundRect key = GetKeyRoundRect(topLeft, i, m_style);
			font(Labels[i]).drawAt(fontSize, key.center().movedBy(0, -(m_style.keySize.y / 36.0)), m_style.textColor);
		}
	}

	inline void Numpad::open(const Vec2& pos) noexcept
	{
		m_pos = pos;
		m_isOpen = true;
		m_overwrite = (not m_style.append);
	}

	inline void Numpad::open(const Vec2& pos, const double value) noexcept
	{
		m_pos = pos;
		m_buffer = Format(FormatData::DecimalPlaces{ m_style.maxDigits }, value);
		m_isOpen = true;
		m_overwrite = (not m_style.append);
	}

	inline void Numpad::close() noexcept
	{
		m_isOpen = false;
	}

	inline bool Numpad::isOpen() const noexcept
	{
		return m_isOpen;
	}

	inline const Vec2& Numpad::getPos() const noexcept
	{
		return m_pos;
	}

	inline void Numpad::setPos(const Vec2& pos) noexcept
	{
		m_pos = pos;
	}

	inline RectF Numpad::region() const noexcept
	{
		const double keyStepX = (m_style.keySize.x + m_style.keyMargin.x);
		const double keyStepY = (m_style.keySize.y + m_style.keyMargin.y);
		return{ m_pos, (keyStepX * 4 + m_style.keyMargin.x), (keyStepY * 4 + m_style.keyMargin.y) };
	}

	inline const Numpad::Style& Numpad::getStyle() const noexcept
	{
		return m_style;
	}

	inline int32 Numpad::getMaxDigits() const noexcept
	{
		return m_style.maxDigits;
	}

	inline void Numpad::setStyle(const Style& style) noexcept
	{
		m_style = style;
	}
	inline void Numpad::setValue(const double value) noexcept
	{
		m_buffer = U"{}"_fmt(value);
	}

	inline void Numpad::clear() noexcept
	{
		m_buffer.clear();
	}

	inline String Numpad::getText() const noexcept
	{
		return getText(false);
	}

	inline String Numpad::withThousandsSeparators() const noexcept
	{
		return getText(true);
	}

	inline double Numpad::getFloat() const noexcept
	{
		return ParseOr<double>(getText(), 0.0);
	}

	inline int64 Numpad::getInt() const noexcept
	{
		return static_cast<int64>(ParseOr<double>(getText(), 0));
	}

	inline String Numpad::formatValue(const double value) const noexcept
	{
		const int32 intDigits = static_cast<int32>(ToString(std::abs(static_cast<int64>(value))).length());
		return Format(FormatData::DecimalPlaces{ (m_style.maxDigits - intDigits) }, value);
	}
}
