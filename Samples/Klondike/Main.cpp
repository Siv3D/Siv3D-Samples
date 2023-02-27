# include <Siv3D.hpp> // OpenSiv3D v0.6.6

/// @brief カードの配列, 末尾にあるカードがテーブル上で一番上にある
using CardList = Array<PlayingCard::Card>;

/// @brief ドラッグ中のカードを管理するクラス
class CardDragger
{
public:

	/// @brief ドラッグを開始します。
	/// @param source ドラッグ元の配列
	/// @param n ドラッグする枚数
	/// @param cardPos カードの座標
	void drag(CardList& source, const int32 n, const Vec2& cardPos)
	{
		cancel();

		m_items.assign((source.end() - n), source.end());

		source.pop_back_N(n);

		m_pSource = &source;

		m_offsetFromCursor = (cardPos - Cursor::PosF());
	}

	/// @brief 現在のドラッグを中止し, ドラッグしていたカードを元の配列に戻します。
	void cancel()
	{
		if (not m_pSource)
		{
			return;
		}

		m_pSource->append(m_items);

		clear();
	}

	/// @brief ドラッグしていたカードをドロップします。
	/// @param target ドロップ先の配列
	void drop(CardList& target)
	{
		if (not m_pSource)
		{
			return;
		}

		target.append(m_items);

		clear();
	}

	/// @brief アイテムを消去します。
	void clear()
	{
		m_items.clear();
		m_pSource = nullptr;
	}

	/// @brief ドラッグ中であるかを返します。
	/// @return ドラッグ中である場合 true, それ以外の場合は false
	[[nodiscard]]
	bool hasItem() const
	{
		return (m_pSource != nullptr);
	}

	/// @brief ドラッグ中のカード一覧を返します。
	/// @return ドラッグ中のカード一覧
	[[nodiscard]]
	const CardList& items() const
	{
		return m_items;
	}

	/// @brief ドラッグ中のカードを描画します。
	/// @param pack 描画するカード
	void draw(const PlayingCard::Pack& pack, const double tableauPileOffset) const
	{
		if (not m_pSource)
		{
			return;
		}

		Vec2 pos = (Cursor::PosF() + m_offsetFromCursor);

		for (const auto& card : m_items)
		{
			pack(card).draw(pos);
			pos.y += tableauPileOffset;
		}
	}

private:

	/// @brief ドラッグ中のカード
	CardList m_items;

	/// @brief ドラッグ元の配列へのポインタ
	CardList* m_pSource = nullptr;

	/// @brief カーソル位置からのずれ（ピクセル）
	Vec2 m_offsetFromCursor{ 0, 0 };
};

/// @brief 勝利アニメーションのクラス
class VictoryAnimation
{
public:

	/// @brief アニメーションの初期パラメータを設定します。
	/// @param suits 組札エリアのスートの並び
	void start(const std::array<PlayingCard::Suit, 4>& suits, const std::array<RectF, 4>& foundationRegions)
	{
		m_time = 0.0;
		m_suits = suits;

		for (size_t i = 0; i < m_cards.size(); ++i)
		{
			for (auto& card : m_cards[i])
			{
				card.reset(foundationRegions[i].pos);
			}
		}
	}

	/// @brief アニメーションを更新します。
	void update(const double deltaTime = Scene::DeltaTime())
	{
		m_time += deltaTime;

		for (size_t i = 0; i < m_cards.size(); ++i)
		{
			if (m_time < (i + 1))
			{
				break;
			}

			for (auto& card : m_cards[i])
			{
				card.update(deltaTime);
			}
		}
	}

	/// @brief アニメーションを描画します。
	/// @param pack 描画するカード
	void draw(const PlayingCard::Pack& pack) const
	{
		for (size_t i = 0; i < m_cards.size(); ++i)
		{
			for (int32 k = 0; k < m_cards[i].size(); ++k)
			{
				pack(PlayingCard::Card{ m_suits[i], (k + 1)})
					.draw(m_cards[i][k].position, m_cards[i][k].angle);
			}
		}
	}

private:

	double m_time = 0.0;

	struct CardState
	{
		static constexpr Vec2 MaxVelocity{ 300, 500 };

		static constexpr double MaxAngularVelocity = 2_pi;

		static constexpr double Gravity = 200;

		Vec2 position;

		Vec2 velocity;

		double angle;

		double angularVelocity;

		void reset(const Vec2& initialPos)
		{
			position = initialPos;
			velocity = Vec2{ Random(-MaxVelocity.x, MaxVelocity.x), Random(-MaxVelocity.y, MaxVelocity.y) };
			angle = 0.0;
			angularVelocity = Random(-MaxAngularVelocity, MaxAngularVelocity);
		}

		void update(const double deltaTime)
		{
			velocity.y = Min(MaxVelocity.y, (velocity.y + Gravity * deltaTime));
			position += (velocity * deltaTime);
			angle += (angularVelocity * deltaTime);
			while (position.x < -200) position.x += (Scene::Width() + 400);
			while (position.x > Scene::Width() + 200) position.x -= (Scene::Width() + 400);
			while (position.y < -200) position.y += (Scene::Height() + 400);
			while (position.y > Scene::Height() + 200) position.y -= (Scene::Height() + 400);
			while (angle < -Math::Pi) angle += 2_pi;
			while (angle > Math::Pi) angle -= 2_pi;
		}
	};

	std::array<PlayingCard::Suit, 4> m_suits;

	std::array<std::array<CardState, 13>, 4> m_cards;
};

/// @brief クロンダイクのゲーム管理クラス
class Klondike
{
public:

	/// @brief ゲームを作成します。
	Klondike()
		: m_pack{ CardWidth }
	{
		reset();
	}

	/// @brief ゲームを初期化します。
	void reset()
	{
		// ドラッグ用パラメータを初期化する
		m_dragger.clear();

		// ジョーカー 0 枚, すべて裏面で山札を初期化する
		m_stock = PlayingCard::CreateDeck(0, PlayingCard::Card::Back).shuffled();

		// 捨て札を空の配列にする
		m_waste.clear();

		// すべての組札を空の配列にする
		m_foundations.fill({});

		// 山札から配って場札を初期化する
		{
			// 配る枚数
			int32 n = 1;

			// 各場札列について
			for (auto& pile : m_tableauPiles)
			{
				// 山札の末尾 n 枚を場札列にセットする
				pile.assign((m_stock.end() - n), m_stock.end());

				// 山札の末尾 n 枚を削除する
				m_stock.pop_back_N(n);

				// 場札列の 1 枚を表向きにする
				pile.back().flip();

				++n;
			}
		}
	}

	/// @brief ゲームに勝利したかを返します。
	/// @return ゲームに勝利した場合 true, それ以外の場合は false
	[[nodiscard]]
	bool isCleared() const
	{
		// すべての組札列に 13 枚のカードがあれば勝利
		for (const auto& foundation : m_foundations)
		{
			if (foundation.size() != 13)
			{
				return false;
			}
		}

		return true;
	}

	/// @brief ゲームを更新します。
	void update()
	{
		if (isCleared()) // もしクリアしていれば
		{
			m_victoryAnimation.update();
			return;
		}

		// カードの状態を更新する
		updateCards();

		// もしクリアしたら
		if (isCleared())
		{
			// クリアアニメーションを開始する
			m_victoryAnimation.start({
				m_foundations[0].back().suit,
				m_foundations[1].back().suit,
				m_foundations[2].back().suit,
				m_foundations[3].back().suit,
			}, FoundationRegions);
		}
	}

	/// @brief ゲームを描画します。
	void draw() const
	{
		drawTable();

		// クリアアニメーションを描画する
		if (isCleared())
		{
			m_victoryAnimation.draw(m_pack);
			return;
		}

		// 山札を描画する
		if (m_stock)
		{
			m_pack(m_stock.back()).drawBack(StockRegion.pos);
		}

		// 捨て札を描画する
		if (m_waste)
		{
			m_pack(m_waste.back()).draw(WasteRegion.pos);
		}

		// 組札を描画する
		for (size_t i = 0; i < m_foundations.size(); ++i)
		{
			if (const auto& foundation = m_foundations[i])
			{
				m_pack(foundation.back()).draw(FoundationRegions[i].pos);
			}
		}

		// 場札を描画する
		for (size_t i = 0; i < m_tableauPiles.size(); ++i)
		{
			Vec2 pos = TableauBottomRegions[i].pos;

			for (const auto& card : m_tableauPiles[i])
			{
				m_pack(card).draw(pos);
				pos.y += TableauPileOffset;
			}
		}

		// ドラッグ中のカードの描画
		m_dragger.draw(m_pack, TableauPileOffset);

		if (shouldChangeCursor())
		{
			// カーソルを手の形状にする
			Cursor::RequestStyle(CursorStyle::Hand);
		}
	}

private:

	/// @brief カードの幅（ピクセル）
	static constexpr double CardWidth = 80;

	/// @brief カードのサイズ（ピクセル）
	static constexpr Vec2 CardSize{ CardWidth, CardWidth * Math::Phi };

	/// @brief 重なった場札の表示オフセット（ピクセル）
	static constexpr double TableauPileOffset = 25;

	/// @brief 山札を置く領域
	static constexpr RectF StockRegion{ Arg::center(100, 100), CardSize };

	/// @brief 捨て札を置く領域
	static constexpr RectF WasteRegion{ Arg::center(200, 100), CardSize };

	/// @brief 組札を置く領域
	static constexpr std::array<RectF, 4> FoundationRegions{
		RectF{ Arg::center(400, 100), CardSize },
		RectF{ Arg::center(500, 100), CardSize },
		RectF{ Arg::center(600, 100), CardSize },
		RectF{ Arg::center(700, 100), CardSize },
	};

	/// @brief 先頭の場札の領域
	static constexpr std::array<RectF, 7> TableauBottomRegions{
		RectF{ Arg::center(100, 250), CardSize },
		RectF{ Arg::center(200, 250), CardSize },
		RectF{ Arg::center(300, 250), CardSize },
		RectF{ Arg::center(400, 250), CardSize },
		RectF{ Arg::center(500, 250), CardSize },
		RectF{ Arg::center(600, 250), CardSize },
		RectF{ Arg::center(700, 250), CardSize },
	};

	/// @brief カードの描画用情報
	PlayingCard::Pack m_pack;

	/// @brief 山札
	CardList m_stock;

	/// @brief 捨て札
	CardList m_waste;

	/// @brief 場札列
	std::array<CardList, 7> m_tableauPiles;

	/// @brief 組札列
	std::array<CardList, 4> m_foundations;

	/// @brief ドラッグ中のカードの管理
	CardDragger m_dragger;

	/// @brief 勝利時のアニメーション
	VictoryAnimation m_victoryAnimation;

	/// @brief 各エリアの枠を描きます。
	void drawTable() const
	{
		// 山札エリアの枠を描画する
		StockRegion.drawFrame(5, ColorF{ Palette::White, 0.2 });

		// 捨て札エリアの枠を描画する
		WasteRegion.drawFrame(5, ColorF{ Palette::White, 0.2 });

		// 組札エリアの枠を描画する
		for (const auto& region : FoundationRegions)
		{
			region.drawFrame(5, ColorF{ Palette::White, 0.2 });
		}

		// 捨て札の回収ボタンを描画する
		if ((not m_stock) && m_waste)
		{
			SimpleGUI::GetFont()(U"\U000F17B4")
				.drawAt(60, StockRegion.center(), ColorF{ 1.0, 0.5 });
		}
	}

	/// @brief カーソルの形状を手のアイコンにする必要があるかを返します。
	/// @return カーソルの形状を手のアイコンにする必要がある場合 true, それ以外の場合は false
	[[nodiscard]]
	bool shouldChangeCursor() const
	{
		// ドラッグ中である
		if (m_dragger.hasItem())
		{
			return true;
		}

		// 山札か捨て札の上にある
		if (((m_stock || m_waste) && StockRegion.mouseOver())
			|| (m_waste && WasteRegion.mouseOver()))
		{
			return true;
		}

		// 組札の上にある
		for (size_t i = 0; i < m_foundations.size(); ++i)
		{
			if (const auto& foundation = m_foundations[i])
			{
				if (FoundationRegions[i].mouseOver())
				{
					return true;
				}
			}
		}

		// 操作可能な場札の上にある
		for (size_t i = 0; i < m_tableauPiles.size(); ++i)
		{
			const auto& pile = m_tableauPiles[i];
			RectF region = TableauBottomRegions[i].movedBy(0, (TableauPileOffset * pile.size()));

			for (int32 k = static_cast<int32>(pile.size() - 1); 0 <= k; --k)
			{
				auto& card = pile[k];
				region.y -= TableauPileOffset;

				if ((not card.isFaceSide) && (k != static_cast<int32>(pile.size() - 1)))
				{
					break;
				}

				if (region.mouseOver())
				{
					return true;
				}
			}
		}

		return false;
	}

	/// @brief カードを更新します。
	void updateCards()
	{
		// もし山札がクリックされたら
		if (StockRegion.leftClicked())
		{
			if (not m_stock) // もし山札を最後までめくっていたら
			{
				// 捨て札を山札に戻す
				m_stock = m_waste.reversed();
				m_waste.clear();

				// 山札はすべて裏向きにする
				for (auto& card : m_stock)
				{
					card.flip();
				}
			}
			else
			{
				// 山札を 1 枚捨て札に送る
				m_waste << m_stock.back();
				m_stock.pop_back();

				// 新しい捨て札は表向きにする
				m_waste.back().flip();
			}

			return;
		}

		// カードのドラッグ開始
		if ((not m_dragger.hasItem()) && MouseL.down())
		{
			// 捨て札の移動
			if (m_waste && WasteRegion.leftClicked())
			{
				// 一番上の捨て札をドラッグ開始する
				m_dragger.drag(m_waste, 1, WasteRegion.pos);
				return;
			}

			// 組札の移動
			for (size_t i = 0; i < m_foundations.size(); ++i)
			{
				const auto& region = FoundationRegions[i];
				auto& foundation = m_foundations[i];

				// もし組札がクリックされたら
				if (foundation && region.leftClicked())
				{
					// 組札の一番上をドラッグ開始
					m_dragger.drag(foundation, 1, region.pos);
					return;
				}
			}

			// 場札の移動
			for (size_t i = 0; i < m_tableauPiles.size(); ++i)
			{
				auto& pile = m_tableauPiles[i];
				RectF region = TableauBottomRegions[i].movedBy(0, (TableauPileOffset * pile.size()));

				for (int32 k = static_cast<int32>(pile.size() - 1); 0 <= k; --k)
				{
					auto& card = pile[k];
					region.y -= TableauPileOffset;

					// もし場札のカードがクリックされたら
					if (region.leftClicked())
					{
						if (card.isFaceSide) // もしそのカードが表なら
						{
							// そのカードからドラッグ開始
							m_dragger.drag(pile, static_cast<int32>(pile.size() - k), region.pos);
						}
						else if (k == static_cast<int32>(pile.size() - 1)) // そうでなく、それが一番上のカードなら
						{
							// そのカードを表に向ける
							card.isFaceSide = true;
						}

						return;
					}
				}
			}
		}

		// ドラッグ中のカードのドロップ
		if (m_dragger.hasItem() && MouseL.up())
		{
			// ドラッグ中のカードの領域
			const RectF dragRegion{ Arg::center = Cursor::PosF(), CardSize };

			// ドロップしたカードの先頭カード
			const auto& droppedFront = m_dragger.items().front();

			// 組札へのドロップ
			if (m_dragger.items().size() == 1)
			{
				for (size_t i = 0; i < m_foundations.size(); ++i)
				{
					const auto& region = FoundationRegions[i];
					auto& foundation = m_foundations[i];

					// もし組札とドラッグ中のカードが重なっていたら
					if (region.intersects(dragRegion))
					{
						if ((foundation.isEmpty() && droppedFront.isAce())
							|| (foundation && (foundation.back().suit == droppedFront.suit) && ((foundation.back().rank + 1) == droppedFront.rank)))
						{
							// 組札の一番上に置く
							m_dragger.drop(foundation);
							return;
						}
					}
				}
			}

			// 場札へのドロップ
			for (size_t i = 0; i < m_tableauPiles.size(); ++i)
			{
				auto& pile = m_tableauPiles[i];
				RectF region = TableauBottomRegions[i].movedBy(0, TableauPileOffset * pile.size());

				// もし新しい場札の領域とドラッグ中のカードが重なっていたら
				if (region.intersects(dragRegion))
				{
					if ((pile.isEmpty() && droppedFront.isKing())
						|| (pile && (pile.back().isBlack() != droppedFront.isBlack()) && ((pile.back().rank - 1) == droppedFront.rank)))
					{
						// 場札の一番上に置く
						m_dragger.drop(pile);
						return;
					}
				}
			}

			// ドロップ先が無かった場合, ドラッグを中止
			m_dragger.cancel();
		}
	}
};

void Main()
{
	// 画面サイズを設定する
	Window::Resize(800, 800);

	// 背景色を設定する
	Scene::SetBackground(Palette::Darkgreen);

	// クロンダイクのゲームを作成する
	Klondike game;

	while (System::Update())
	{
		game.update();

		game.draw();

		if (SimpleGUI::Button(U"New Game", Vec2{ 40, 740 }))
		{
			game.reset();
		}
	}
}
