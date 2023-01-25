# include <Siv3D.hpp>
# include <list>


// カードサイズ
constexpr double CardWidth = 80;
constexpr Vec2 CardSize{ CardWidth, CardWidth * Math::Phi };
// 山札の領域
constexpr RectF StockRegion{ Arg::center = Vec2{ 100, 100 }, CardSize };
// 捨て札の領域
constexpr RectF WasteRegion{ Arg::center = Vec2{ 200, 100 }, CardSize };
// 組札の領域
constexpr RectF FoundationRegions[4] {
	RectF{ Arg::center = Vec2{ 400, 100 }, CardSize },
	RectF{ Arg::center = Vec2{ 500, 100 }, CardSize },
	RectF{ Arg::center = Vec2{ 600, 100 }, CardSize },
	RectF{ Arg::center = Vec2{ 700, 100 }, CardSize },
};
// 一番下の場札の領域
constexpr RectF TableauBottomRegions[7] {
	RectF{ Arg::center = Vec2{ 100, 250 }, CardSize },
	RectF{ Arg::center = Vec2{ 200, 250 }, CardSize },
	RectF{ Arg::center = Vec2{ 300, 250 }, CardSize },
	RectF{ Arg::center = Vec2{ 400, 250 }, CardSize },
	RectF{ Arg::center = Vec2{ 500, 250 }, CardSize },
	RectF{ Arg::center = Vec2{ 600, 250 }, CardSize },
	RectF{ Arg::center = Vec2{ 700, 250 }, CardSize },
};
// 場札のずらし幅
constexpr double TableauPileOffset = 25;


// カードを扱うコンテナ
using CardList = std::list<PlayingCard::Card>;


// カードをドラッグするためのクラス
class CardDragger
{
private:
	// ドラッグ中のカード
	CardList cards;
	// ドラッグ元のリスト
	CardList* source = nullptr;
	// ドラッグ元のイテレータ
	CardList::iterator sourcePos;
	// カーソル位置からのずれ
	Vec2 offsetFromCursor;

public:
	// ドラッグ開始
	void dragStart(CardList& source, CardList::iterator pos, const Vec2& cardPos)
	{
		dragStart(source, pos, std::next(pos), cardPos);
	}
	void dragStart(CardList& source, CardList::iterator first, CardList::iterator last, const Vec2& cardPos)
	{
		dragEnd();
		cards.splice(cards.end(), source, first, last);
		this->source = &source;
		sourcePos = last;
		offsetFromCursor = cardPos - Cursor::PosF();
	}

	// ドラッグ終了
	void dragEnd()
	{
		if (source)
		{
			source->splice(sourcePos, cards, cards.begin(), cards.end());
			source = nullptr;
		}
	}

	// ドロップ
	void drop(CardList& target, CardList::iterator targetPos)
	{
		if (source)
		{
			target.splice(targetPos, cards, cards.begin(), cards.end());
			source = nullptr;
		}
	}

	// 初期化
	void clear()
	{
		cards.clear();
		source = nullptr;
	}

	// ドラッグ中かどうか
	[[nodiscard]]
	bool isDragging() const
	{
		return source != nullptr;
	}

	// ドラッグ中のカード
	[[nodiscard]]
	const CardList& cardList() const
	{
		return cards;
	}

	// ドラッグ中のカードを描画
	void draw(const PlayingCard::Pack& pack) const
	{
		if (source)
		{
			auto pos = Cursor::PosF() + offsetFromCursor;
			for (auto&& card : cards)
			{
				pack(card).draw(pos);
				pos.y += TableauPileOffset;
			}
		}
	}
};


// クリアアニメーションのためのクラス
class ClearAnimation
{
private:
	static constexpr Vec3 MaxVelocity{ 300, 500, Math::TwoPi };
	static constexpr double Gravity = 200;

	const Font font{ 120, Typeface::Black };
	double time;
	struct
	{
		PlayingCard::Suit suit;
		Vec3 positions[13];
		Vec3 velocity[13];
	}
	foundations[4];

public:
	// アニメーション開始
	void start(const PlayingCard::Suit (&suits)[4])
	{
		time = 0;
		for (auto i : step(4))
		{
			auto&& [suit, positions, velocity] = foundations[i];
			suit = suits[i];
			for (auto&& r : positions)
			{
				r = Vec3{ FoundationRegions[i].pos, 0 };
			}
			for (auto&& v : velocity)
			{
				auto&& [x, y, theta] = MaxVelocity;
				v = Vec3{ Random(-x, x), Random(-y, y), Random(-theta, theta) };
			}
		}
	}

	// 更新
	void update()
	{
		time += Scene::DeltaTime();
		for (auto i : step(Min(4, static_cast<int>(time))))
		{
			auto&& [suit, positions, velocity] = foundations[i];
			for (auto&& v : velocity)
			{
				v.y = Min(MaxVelocity.y, v.y + Gravity * Scene::DeltaTime());
			}
			for (auto i : step(13))
			{
				auto&& r = positions[i];
				auto&& v = velocity[i];
				r += v * Scene::DeltaTime();
				while (r.x < -200) r.x += Scene::Width() + 400;
				while (r.x > Scene::Width() + 200) r.x -= Scene::Width() + 400;
				while (r.y < -200) r.y += Scene::Height() + 400;
				while (r.y > Scene::Height() + 200) r.y -= Scene::Height() + 400;
				while (r.z < -Math::Pi) r.z += Math::TwoPi;
				while (r.z > Math::Pi) r.z -= Math::TwoPi;
			}
		}
	}

	// 描画
	void draw(const PlayingCard::Pack& pack) const
	{
		for (auto&& [suit, positions, velocity] : foundations)
		{
			for (auto i : step(13))
			{
				auto&& [x, y, theta] = positions[i];
				pack(PlayingCard::Card{ suit, i + 1 }).draw(x, y, theta);
			}
		}
		font(U"おめでとう").drawAt(font.fontSize() - Periodic::Sine0_1(800ms) * 20, Scene::Center(), Palette::Red);
	}
};


// クロンダイクの処理、描画のためのクラス
class Klondike
{
private:
	// カード描画用
	const PlayingCard::Pack pack{ CardWidth };
	// 絵文字描画用
	const Font emoji{ 30 , Typeface::MonochromeEmoji };

	// 山札（捨て札も含む）
	CardList stock;
	// 山札の一番上の位置（これより前が捨て札）
	CardList::iterator stockTop = stock.begin();
	// 場札
	CardList tableauPiles[7];
	// 組札
	CardList foundations[4];
	// ドラッグ用
	CardDragger dragger;
	// クリアフラグ
	bool cleared = false;
	// クリアアニメーション用
	ClearAnimation clearAnimation;

public:
	// クリアチェック
	[[nodiscard]]
	bool isCleared() const
	{
		// すべての組札にカードが13枚あればクリア
		for (auto&& foundation : foundations)
		{
			if (foundation.size() != 13)
			{
				return false;
			}
		}
		return true;
	}

	// 開始
	void start()
	{
		// デッキ生成
		auto deck = PlayingCard::CreateDeck();
		deck.shuffle();
		// 山札を初期化
		stock.assign(deck.begin(), deck.end());
		// 組札を初期化
		for (auto&& foundation : foundations)
		{
			foundation.clear();
		}
		// 山札から配って場札を初期化
		for (auto i : step(7))
		{
			auto& pile = tableauPiles[i];
			pile.clear();
			pile.splice(pile.end(), stock, stock.begin(), std::next(stock.begin(), i + 1));
			std::for_each(pile.begin(), std::prev(pile.end()), [](auto&& card) { card.isFaceSide = false; });
		}
		// 山札の一番上の位置を設定
		stockTop = stock.begin();
		// ドラッグ用パラメータの初期化
		dragger.clear();
		// クリアフラグの消去
		cleared = false;
	}

	// 更新
	void update()
	{
		// もし [リスタート] が押されたら
		if (SimpleGUI::Button(U"リスタート", Vec2{ 40, 740 }))
		{
			// ゲームを開始
			start();
			return;
		}

		// もしクリアしていれば
		if (cleared)
		{
			// クリアアニメーションを更新
			clearAnimation.update();
		}
		// そうでなければ
		else
		{
			// カードを更新
			updateCards();

			// もしマウスの左ボタンが離されたら
			if (MouseL.up())
			{
				// ドラッグ終了
				dragger.dragEnd();
			}

			// もしクリアしたら
			if (isCleared())
			{
				// クリアフラグを立てる
				cleared = true;
				// クリアアニメーションを開始
				clearAnimation.start({
					foundations[0].back().suit,
					foundations[1].back().suit,
					foundations[2].back().suit,
					foundations[3].back().suit,
				});
			}
		}
	}

	// 描画
	void draw() const
	{
		// 枠の描画
		StockRegion.drawFrame(5, ColorF{ Palette::White, 0.2 });
		WasteRegion.drawFrame(5, ColorF{ Palette::White, 0.2 });
		for (auto&& region : FoundationRegions)
		{
			region.drawFrame(5, ColorF{ Palette::White, 0.2 });
		}

		// クリアアニメーションの描画
		if (cleared)
		{
			clearAnimation.draw(pack);
			return;
		}

		// 山札の描画
		if (stock.size())
		{
			if (stockTop == stock.end())
			{
				emoji(U'🔃').drawAt(StockRegion.center(), ColorF{ Palette::White, 0.5 });
			}
			else
			{
				pack(stock.front()).drawBack(StockRegion.pos);
			}
		}

		// 捨て札の描画
		if (stockTop != stock.begin())
		{
			pack(*std::prev(stockTop)).draw(WasteRegion.pos);
		}

		// 組札の描画
		for (auto i : step(4))
		{
			auto&& region = FoundationRegions[i];
			auto&& foundation = foundations[i];
			if (foundation.size())
			{
				pack(foundation.back()).draw(region.pos);
			}
		}

		// 場札の描画
		for (auto i : step(7))
		{
			auto&& pile = tableauPiles[i];
			Vec2 pos = TableauBottomRegions[i].pos;
			for (auto&& card : pile)
			{
				pack(card).draw(pos);
				pos.y += TableauPileOffset;
			}
		}

		// ドラッグ中のカードの描画
		dragger.draw(pack);
	}

private:
	// カードの更新
	void updateCards()
	{
		// もしマウスの操作がなければ
		if (not MouseL.down() && not MouseL.up())
		{
			// 何もせず終了
			return;
		}

		// もし山札がクリックされたら
		if (StockRegion.leftClicked())
		{
			// もし山札を最後までめくっていたら
			if (stockTop == stock.end())
			{
				// 山札をもとに戻す
				stockTop = stock.begin();
			}
			// そうでなければ
			else
			{
				// 一枚めくる
				++stockTop;
			}
			return;
		}

		// もし捨て札がクリックされたら
		if (WasteRegion.leftClicked())
		{
			// もし捨て札があれば
			if (stockTop != stock.begin())
			{
				// 一番上の捨て札をドラッグする
				dragger.dragStart(stock, std::prev(stockTop), WasteRegion.pos);
			}
			return;
		}

		// ドラッグ中のカードの領域
		const RectF dragRegion{ Arg::center = Cursor::PosF(), CardSize };

		for (auto i : step(4))
		{
			auto&& region = FoundationRegions[i];
			auto&& foundation = foundations[i];

			// もし組札がクリックされたら
			if (region.leftClicked())
			{
				// もし組札があれば
				if (foundation.size())
				{
					// 組札の一番上をドラッグ開始
					dragger.dragStart(foundation, std::prev(foundation.end()), region.pos);
				}
				return;
			}

			// もし組札とドラッグ中のカードが重なっていたら
			if (region.intersects(dragRegion))
			{
				// もし1枚のカードがドロップされたら
				if (MouseL.up() && dragger.isDragging() && dragger.cardList().size() == 1)
				{
					auto&& top = foundation.back();
					auto&& droppedCard = dragger.cardList().front();
					// もし置けるカードなら
					if (foundation.empty()
						? droppedCard.isAce()
						: droppedCard.suit == top.suit && droppedCard.rank == top.rank + 1)
					{
						// 組札の一番上に置く
						dragger.drop(foundation, foundation.end());
						return;
					}
				}
			}
		}

		for (auto i : step(7))
		{
			auto&& pile = tableauPiles[i];
			RectF region = TableauBottomRegions[i].movedBy(0, TableauPileOffset * pile.size());

			// もし新しい場札の領域とドラッグ中のカードが重なっていたら
			if (region.intersects(dragRegion))
			{
				// もしカードがドロップされたら
				if (MouseL.up() && dragger.isDragging())
				{
					auto&& top = pile.back();
					auto&& droppedCard = dragger.cardList().front();
					// もし置けるカードなら
					if (pile.empty()
						? droppedCard.isKing()
						: droppedCard.isBlack() != top.isBlack() && droppedCard.rank == top.rank - 1)
					{
						// 場札の一番上に置く
						dragger.drop(pile, pile.end());
						return;
					}
				}
			}

			for (auto it = pile.rbegin(); it != pile.rend(); ++it)
			{
				region.y -= TableauPileOffset;

				// もし場札のカードがクリックされたら
				if (region.leftClicked())
				{
					// もしそのカードが表なら
					if (it->isFaceSide)
					{
						// そのカードから上をドラッグ開始
						dragger.dragStart(pile, std::prev(it.base()), pile.end(), region.pos);
					}
					// そうでなく、それが一番上のカードなら
					else if (it == pile.rbegin())
					{
						// そのカードを表に向ける
						it->isFaceSide = true;
					}
					return;
				}
			}
		}
	}
};


void Main()
{
	// 画面サイズの設定
	Window::Resize(800, 800);
	// 背景色の設定
	Scene::SetBackground(Palette::Darkgreen);

	// クロンダイク
	Klondike game;
	game.start();

	while (System::Update())
	{
		game.update();
		game.draw();
	}
}
