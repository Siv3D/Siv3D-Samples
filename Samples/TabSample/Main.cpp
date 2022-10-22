# include <Siv3D.hpp> // OpenSiv3D v0.6.5

class ITab
{
public:

	ITab(const SizeF& tabSize, const Array<String>& items)
		: m_tabSize{ tabSize }
		, m_items{ items } {}

	virtual ~ITab() = default;

	/// @brief すべてのタブを描画します。
	/// @param pos タブ描画開始位置
	/// @param font テキスト描画に使うフォント（FontMethod::MSDF 設定で作成）
	/// @param color アクティブなタブの背景色
	/// @param outlineColor タブの枠の色
	virtual void draw(const Vec2& pos, const Font& font, const ColorF& color, const ColorF& outlineColor) const = 0;

	/// @brief タブの個数を返します。
	/// @return タブの個数
	virtual size_t getTabCount() const noexcept
	{
		return m_items.size();
	}

	/// @brief アクティブなタブのインデックスを返します。
	/// @return アクティブなタブのインデックス
	virtual size_t getActiveTabIndex() const noexcept
	{
		return m_activeIndex;
	}

	/// @brief アクティブなタブを変更します。
	/// @param index アクティブにするタブのインデックス
	virtual void setActiveTabIndex(size_t index) noexcept
	{
		assert(index < m_items.size());
		m_activeIndex = index;
	}

	/// @brief タブを左右に移動します。
	/// @param offset 左に移動する場合 -1, 右に移動する場合は +1
	/// @param wrapAround 端まで到達したときに反対側に戻る場合 true, それ以外の場合は false
	virtual void advance(int32 offset, bool wrapAround = false)
	{
		assert(InRange(offset, -1, 1));

		if (offset == -1)
		{
			if (m_activeIndex == 0)
			{
				if (wrapAround)
				{
					m_activeIndex = (m_items.size() - 1);
				}
			}
			else
			{
				--m_activeIndex;
			}
		}
		else if (offset == 1)
		{
			if (m_activeIndex == (m_items.size() - 1))
			{
				if (wrapAround)
				{
					m_activeIndex = 0;
				}
			}
			else
			{
				++m_activeIndex;
			}
		}
	}

protected:

	SizeF m_tabSize;

	Array<String> m_items;

	size_t m_activeIndex = 0;
};

class TabA : public ITab
{
public:

	TabA(const SizeF& tabSize, const Array<String>& items)
		: ITab{ tabSize, items }
	{
		double posX = 0.0;

		for (const auto& item : m_items)
		{
			m_tabPositions.emplace_back(posX, 0);

			posX += (tabSize.x + tabSize.x * 0.14);
		}
	}

	void draw(const Vec2& pos, const Font& font, const ColorF& color, const ColorF& outlineColor) const override
	{
		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos + m_tabPositions[i]), m_tabSize };

			if (i == m_activeIndex)
			{
				tab.draw(color);
			}
			else
			{
				tab.drawFrame(3, 0, outlineColor);
			}
		}

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos + m_tabPositions[i]), m_tabSize };

			font(m_items[i]).drawAt(TextStyle::Shadow(Vec2{ 2.5, 2.5 }, ColorF{ 0.0, 0.6 }), 20, tab.center());
		}
	}

private:

	Array<Vec2> m_tabPositions;
};

class TabB : public ITab
{
public:

	TabB(const SizeF& tabSize, const Array<String>& items)
		: ITab{ tabSize, items }
	{
		double posX = 0.0;

		for (const auto& item : m_items)
		{
			m_tabPositions.emplace_back(posX, 0);

			posX += (tabSize.x + tabSize.x * 0.14);
		}
	}

	void draw(const Vec2& pos, const Font& font, const ColorF& color, const ColorF& outlineColor) const override
	{
		const double radius = (m_tabSize.y * 0.25);

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos + m_tabPositions[i]), m_tabSize };

			if (i == m_activeIndex)
			{
				tab.rounded(radius, radius, 0, 0).draw(color);
			}
			else
			{
				tab.stretched(-1.5).rounded(radius, radius, 0, 0).drawFrame(3, outlineColor);
			}
		}

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos + m_tabPositions[i]), m_tabSize };

			font(m_items[i]).drawAt(TextStyle::Shadow(Vec2{ 2.5, 2.5 }, ColorF{ 0.0, 0.6 }), 20, tab.center());
		}
	}

private:

	Array<Vec2> m_tabPositions;
};

class TabC : public ITab
{
public:

	TabC(const SizeF& tabSize, const Array<String>& items)
		: ITab{ tabSize, items } {}

	void draw(const Vec2& pos, const Font& font, const ColorF& color, const ColorF& outlineColor) const override
	{
		constexpr double Thickness = 3.0;
		const double radius = (m_tabSize.y * 0.5);

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize };

			if (i == 0) // ⊂
			{
				tab.stretched(-Thickness * 0.5).rounded(radius, 0, 0, radius).drawFrame(Thickness, outlineColor);
			}
			else if (i == (m_items.size() - 1)) // ⊃
			{
				tab.stretched(-Thickness * 0.5).rounded(0, radius, radius, 0).drawFrame(Thickness, outlineColor);
			}
			else // □
			{
				tab.drawFrame(Thickness, 0, outlineColor);
			}
		}

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize };

			if (i == m_activeIndex)
			{
				if (i == 0) // ⊂
				{
					tab.rounded(radius, 0, 0, radius).draw(color);
				}
				else if (i == (m_items.size() - 1)) // ⊃
				{
					tab.rounded(0, radius, radius, 0).draw(color);
				}
				else // □
				{
					tab.draw(color);
				}
			}
		}

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize };

			font(m_items[i]).drawAt(TextStyle::Shadow(Vec2{ 2.5, 2.5 }, ColorF{ 0.0, 0.6 }), 20, tab.center());
		}
	}
};

class TabD : public ITab
{
public:

	TabD(const SizeF& tabSize, const Array<String>& items)
		: ITab{ tabSize, items } {}

	void draw(const Vec2& pos, const Font& font, const ColorF& color, const ColorF& outlineColor) const override
	{
		constexpr double Thickness = 3.0;
		const double radius = (m_tabSize.y * 0.5);
		const double smallRadius = (m_tabSize.y * 0.1);

		const double leftX = pos.x;
		const double width = ((m_items.size() - 1) * (m_tabSize.x - Thickness) + m_tabSize.x);

		RectF{ pos.x, pos.y + 3, width, m_tabSize.y - 6}.stretched(-Thickness * 0.5).rounded(radius).drawFrame(Thickness, outlineColor);

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab = RectF{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize.x, m_tabSize.y };

			if (i == m_activeIndex)
			{
				if (i == 0) // ⊂
				{
					tab.rounded(radius, smallRadius, smallRadius, radius).draw(color);
				}
				else if (i == (m_items.size() - 1)) // ⊃
				{
					tab.rounded(smallRadius, radius, radius, smallRadius).draw(color);
				}
				else // □
				{
					tab.rounded(smallRadius).draw(color);
				}
			}
		}

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize };

			font(m_items[i]).drawAt(TextStyle::Shadow(Vec2{ 2.5, 2.5 }, ColorF{ 0.0, 0.6 }), 20, tab.center());
		}
	}
};

class TabE : public ITab
{
public:

	TabE(const SizeF& tabSize, const Array<String>& items)
		: ITab{ tabSize, items } {}

	void draw(const Vec2& pos, const Font& font, const ColorF& color, const ColorF& outlineColor) const override
	{
		constexpr double Thickness = 3.0;
		const double shear = (m_tabSize.y * 0.2);
		const double radius = (m_tabSize.y * 0.5);

		const double leftX = pos.x;
		const double width = ((m_items.size() - 1) * (m_tabSize.x - Thickness) + m_tabSize.x);

		RectF{ pos.x, pos.y + 3, width, m_tabSize.y - 6 }.stretched(-Thickness * 0.5).rounded(radius).drawFrame(Thickness, outlineColor);

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab = RectF{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize.x, m_tabSize.y };

			if (i == m_activeIndex)
			{
				if (i == 0) // ⊂
				{
					tab.stretched(0, -shear, 0, 0).rounded(radius, 0, 0, radius).draw(color);
					Triangle{ tab.tr().movedBy(-shear, 0),  tab.tr().movedBy(shear, 0), tab.br().movedBy(-shear, 0) }.draw(color);
				}
				else if (i == (m_items.size() - 1)) // ⊃
				{
					Triangle{ tab.tl().movedBy(shear, 0), tab.bl().movedBy(shear, 0), tab.bl().movedBy(-shear, 0) }.draw(color);
					tab.stretched(0, 0, 0, -shear).rounded(0, radius, radius, 0).draw(color);
				}
				else // □
				{
					tab.shearedX(shear).draw(color);
				}
			}
		}

		for (size_t i = 0; i < m_items.size(); ++i)
		{
			const RectF tab{ (pos.x + i * (m_tabSize.x - Thickness)), pos.y, m_tabSize };

			// 文字を傾かせる
			const Transformer2D tr{ Mat3x2::ShearX(0.35).translated(tab.center()) };

			font(m_items[i]).drawAt(TextStyle::Shadow(Vec2{ 2.5, 2.5 }, ColorF{ 0.0, 0.6 }), 20, Vec2{ 0, 0 });
		}
	}
};

void Main()
{
	Window::Resize(1280, 720);
	const Font font{ FontMethod::MSDF, 48, Typeface::Heavy };
	const Array<String> items = { U"ステータス", U"武器", U"装備", U"スキル", U"任務", U"プロフィール" };
	constexpr ColorF TabColor{ 0.2, 0.5, 0.9 };
	constexpr ColorF TabOutlineColor{ 0.5 };
	constexpr ColorF ContentColor{ 0.5 };

	TabA tabA{ Size{ 160, 50 }, items };
	TabB tabB{ Size{ 160, 50 }, items };
	TabC tabC{ Size{ 182, 50 }, items };
	TabD tabD{ Size{ 182, 50 }, items };
	TabE tabE{ Size{ 182, 50 }, items };

	while (System::Update())
	{
		if (KeyLeft.down())
		{
			tabA.advance(-1);
			tabB.advance(-1);
			tabC.advance(-1);
			tabD.advance(-1);
			tabE.advance(-1);
		}
		else if (KeyRight.down())
		{
			tabA.advance(+1);
			tabB.advance(+1);
			tabC.advance(+1);
			tabD.advance(+1);
			tabE.advance(+1);
		}

		tabA.draw(Vec2{ 140, 40 }, font, TabColor, TabOutlineColor);
		tabB.draw(Vec2{ 140, 180 }, font, TabColor, TabOutlineColor);
		tabC.draw(Vec2{ 140, 310 }, font, TabColor, TabOutlineColor);
		tabD.draw(Vec2{ 140, 450 }, font, TabColor, TabOutlineColor);
		tabE.draw(Vec2{ 140, 590 }, font, TabColor, TabOutlineColor);

		RectF{ 120, 100, 1120, 50 }.draw(ContentColor);
		RectF{ 120, 230 - 2, 1120, 50 }.draw(ContentColor);
		RectF{ 120, 370, 1120, 50 }.draw(ContentColor);
		RectF{ 120, 510, 1120, 50 }.draw(ContentColor);
		RectF{ 120, 650, 1120, 50 }.draw(ContentColor);
	}
}
