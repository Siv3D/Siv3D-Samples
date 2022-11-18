# include <Siv3D.hpp> // OpenSiv3D v0.6.5

/// @brief HP バー
class HPBar
{
public:

	/// @brief 表示スタイル
	struct Style
	{
		/// @brief バーの背景色
		ColorF backgroundColor{ 0.0, 0.6 };

		/// @brief 遅延 HP の色
		ColorF delayColor{ 0.9, 0.8, 0.3 };

		/// @brief HP の色
		ColorF hpColor{ 0.8, 0.2, 0.2 };

		/// @brief 枠の色
		ColorF frameColor{ 0.1 };

		/// @brief 枠の太さ（ピクセル）
		double frameThickness = 1.5;
	};

	HPBar() = default;

	/// @brief HP バーを作成します。
	/// @param maxHP 最大 HP
	explicit constexpr HPBar(int32 maxHP) noexcept
		: m_maxHP{ maxHP }
		, m_currentHP{ maxHP }
		, m_delayHP{ static_cast<double>(m_currentHP) } {}

	/// @brief HP バーを作成します。
	/// @param maxHP 最大 HP
	/// @param currentHP 現在の HP
	constexpr HPBar(int32 maxHP, int32 currentHP) noexcept
		: m_maxHP{ maxHP }
		, m_currentHP{ currentHP }
		, m_delayHP{ static_cast<double>(m_currentHP) } {}

	/// @brief HP バーをアニメーションします。
	/// @param smoothTimeSec 遅延 HP の平滑化時間（秒）
	void update(double smoothTimeSec = 0.4)
	{
		m_delayHP = Math::SmoothDamp(m_delayHP, m_currentHP, m_delayVelocity, smoothTimeSec);
	}

	/// @brief 長方形の HP バーを描画します。
	/// @param rect 長方形
	/// @param style スタイル
	void draw(const RectF& rect, const Style& style = {})
	{
		const RectF rectDelay{ rect.pos, (rect.w * getDelayHPRatio()), rect.h };
		const RectF rectHP{ rect.pos, (rect.w * getHPRatio()), rect.h };

		rect.draw(style.backgroundColor);
		rectDelay.draw(style.delayColor);
		rectHP.draw(style.hpColor);
		rect.drawFrame(style.frameThickness, style.frameColor);
	}

	/// @brief 指定した長方形の角を削った、六角形の HP バーを描画します。
	/// @param rect 長方形
	/// @param style スタイル
	void drawHex(const RectF& rect, const Style& style = {})
	{
		const RectF rectDelay{ rect.pos, (rect.w * getDelayHPRatio()), rect.h };
		const RectF rectHP{ rect.pos, (rect.w * getHPRatio()), rect.h };
		const Polygon hex = MakeHexPolygon(rect);

		hex.draw(style.backgroundColor);

		for (const auto& shape : Geometry2D::And(hex, rectDelay))
		{
			shape.draw(style.delayColor);
		}

		for (const auto& shape : Geometry2D::And(hex, rectHP))
		{
			shape.draw(style.hpColor);
		}

		hex.drawFrame(style.frameThickness, style.frameColor);
	}

	/// @brief 現在の HP を返します。
	/// @return 現在の HP
	[[nodiscard]]
	constexpr int32 getHP() const noexcept
	{
		return m_currentHP;
	}

	/// @brief 最大 HP を返します。
	/// @return 最大 HP
	[[nodiscard]]
	constexpr int32 getMaxHP() const noexcept
	{
		return m_maxHP;
	}

	/// @brief 最大 HP に対する現在の HP の割合を返します。
	/// @return 最大 HP に対する現在の HP の割合
	[[nodiscard]]
	constexpr double getHPRatio() const noexcept
	{
		return (static_cast<double>(m_currentHP) / m_maxHP);
	}

	/// @brief HP を変更します。遅延エフェクトは発生しません。
	/// @param hp 新しい HP
	constexpr void setHP(int32 hp) noexcept
	{
		m_currentHP = Clamp(hp, 0, m_maxHP);
		m_delayHP = m_currentHP;
		m_delayVelocity = 0.0;
	}

	/// @brief HP を減らします。
	/// @param damage 減らす量
	constexpr void damage(int32 damage) noexcept
	{
		m_currentHP = Clamp((m_currentHP - damage), 0, m_maxHP);
	}

	/// @brief HP を回復します。遅延エフェクトは発生しません。
	/// @param heal 回復量
	constexpr void heal(int32 heal) noexcept
	{
		setHP(m_currentHP + heal);
	}

private:

	int32 m_maxHP = 1;

	int32 m_currentHP = 1;

	double m_delayHP = 1;

	double m_delayVelocity = 0.0;

	[[nodiscard]]
	constexpr double getDelayHPRatio() const noexcept
	{
		return (m_delayHP / m_maxHP);
	}

	[[nodiscard]]
	static Polygon MakeHexPolygon(const RectF& rect)
	{
		const Vec2 offsetH{ (rect.h * 0.5), 0.0 };
		const Vec2 offsetV{ 0.0, (rect.h * 0.5) };
		return Polygon{ { (rect.tl() + offsetH), (rect.tr() - offsetH), (rect.tr() + offsetV),
			(rect.br() - offsetH), (rect.bl() + offsetH), (rect.tl() + offsetV) } };
	}
};

void Main()
{
	Scene::SetBackground(ColorF{ 0.8, 0.9, 1.0 });
	Window::Resize(1280, 720);

	Array<HPBar> hpBars =
	{
		HPBar{ 400 }, HPBar{ 1600 }, HPBar{ 6400 },
		HPBar{ 400 }, HPBar{ 1600 }, HPBar{ 6400 },
	};

	while (System::Update())
	{
		for (size_t i = 0; i < hpBars.size(); ++i)
		{
			const double x = (150.0 + (i % 3) * 360.0);
			const double y = (180.0 + (i / 3) * 300.0 + (i % 3) * 50.0);
			const Circle circle{ x, y, 100 };

			hpBars[i].update();

			if (circle.mouseOver())
			{
				Cursor::RequestStyle(CursorStyle::Hand);

				if (MouseL.down())
				{
					hpBars[i].damage(90);
				}
				else if (MouseR.down())
				{
					hpBars[i].heal(100);
				}
			}
		}

		for (size_t i = 0; i < hpBars.size(); ++i)
		{
			const double x = (150.0 + (i % 3) * 360.0);
			const double y = (180.0 + (i / 3) * 300.0 + (i % 3) * 50.0);
			const Circle circle{ x, y, 100 };
			const RectF rect = RectF{ x, y, 300, 16 }.movedBy(40, -120);

			circle.drawFrame(2);

			if (i < 3)
			{
				hpBars[i].draw(rect);
			}
			else
			{
				hpBars[i].drawHex(rect);
			}
		}
	}
}
