# include <Siv3D.hpp> // OpenSiv3D v0.6.5

namespace OthelloAI
{
	// ビットボード
	using BitBoard = uint64;

	/// @brief セルのインデックス
	/// @remark A1 が 0, B1 が 1, C1 が 2, ... H8 が 63 
	using CellIndex = int32;

	/// @brief ビットボード上の指定したセルにフラグが立っているかを返します。
	/// @param bitBoard ビットボード
	/// @param cellIndex セルのインデックス
	/// @return フラグが立っている場合 true, それ以外の場合は false
	constexpr bool HasFlag(BitBoard bitBoard, CellIndex cellIndex)
	{
		return static_cast<bool>(1 & (bitBoard >> (63 - cellIndex)));
	}

	// AI 非同期タスクの中断フラグ
	std::atomic<bool> g_abort = false;

	// 石を返すときに使う情報
	struct Flip
	{
		// 返る石
		BitBoard flip;

		// 着手位置
		uint8 pos;
	};

	// 64ビット整数の1のビットの個数を数える
	constexpr int32 pop_count_ull(uint64 x)
	{
		x = x - ((x >> 1) & 0x5555555555555555ULL);
		x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
		x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
		x = (x * 0x0101010101010101ULL) >> 56;
		return static_cast<int32>(x);
	}

	// 2進数として数値を見て右端からいくつ0が連続しているか: Number of Training Zero
	inline uint_fast8_t ntz(uint64* x)
	{
		return static_cast<uint_fast8_t>(pop_count_ull((~(*x)) & ((*x) - 1)));
	}

	// 立っているビットを走査するときにfor文で使うと便利
	inline uint_fast8_t first_bit(uint64* x)
	{
		return ntz(x);
	}

	// 立っているビットを走査するときにfor文で使うと便利
	inline uint_fast8_t next_bit(uint64* x)
	{
		*x &= *x - 1; // 最右の立っているビットをオフにする
		return ntz(x);
	}

	// ビットボード
	class Board
	{
	public:

		// スコアの絶対値の最大値
		static constexpr int32 MaxScore = 64;

		// 初期局面
		void reset()
		{
			m_player = 0x0000000810000000ULL;
			m_opponent = 0x0000001008000000ULL;
		}

		// 着手
		void move(Flip flip)
		{
			m_player ^= flip.flip;
			m_opponent ^= flip.flip;
			m_player ^= 1ULL << flip.pos;
			std::swap(m_player, m_opponent);
		}

		// 着手を取り消し
		void undo(Flip flip)
		{
			std::swap(m_player, m_opponent);
			m_player ^= 1ULL << flip.pos;
			m_player ^= flip.flip;
			m_opponent ^= flip.flip;
		}

		// 着手したときに返る石を求める
		Flip getFlip(uint8 pos)
		{
			constexpr int32 Shifts[8] = { 1, -1, 8, -8, 7, -7, 9, -9 };
			constexpr uint64 Masks[4] = { 0x7E7E7E7E7E7E7E7EULL, 0x00FFFFFFFFFFFF00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL };
			const uint64 x = (1ULL << pos);
			Flip result{ .flip = 0ULL, .pos = pos };

			// 縦横斜めの8方向それぞれ別に計算する
			for (int32 i = 0; i < 8; ++i)
			{
				result.flip |= get_flip_part(Shifts[i], Masks[i / 2], x);
			}

			return result;
		}

		// パス
		void pass()
		{
			std::swap(m_player, m_opponent);
		}

		// 評価関数(終局していない場合に使う。マスの重みを使った評価で最終石差を推測する)
		int32 evaluate()
		{
			constexpr int32 cell_weight_score[10] = { 2714, 147, 69, -18, -577, -186, -153, -379, -122, -169 };
			constexpr uint64 cell_weight_mask[10] = { 0x8100000000000081ULL, 0x4281000000008142ULL, 0x2400810000810024ULL, 0x1800008181000018ULL, 0x0042000000004200ULL,
				0x0024420000422400ULL, 0x0018004242001800ULL, 0x0000240000240000ULL, 0x0000182424180000ULL, 0x0000001818000000ULL };
			int32 res = 0;

			for (int32 i = 0; i < 10; ++i) // 盤面を10種類のマスに分けてそれぞれのマスに重みをつけたので、1種類ずつ計算
			{
				res += cell_weight_score[i] * (pop_count_ull(m_player & cell_weight_mask[i]) - pop_count_ull(m_opponent & cell_weight_mask[i]));
			}

			res += res > 0 ? 128 : (res < 0 ? -128 : 0);
			res /= 256; // 最終石差の256倍を学習データにしたので、256で割って実際の最終石差の情報にする
			return std::max(-MaxScore, std::min(MaxScore, res)); // -64から+64までの範囲に収める
		}

		/// @brief 現在の手番のビットボードを返します。
		/// @return 現在の手番のビットボード
		[[nodiscard]]
		BitBoard getPlayerBitBoard() const
		{
			return m_player;
		}

		/// @brief 現在の手番でないほうのビットボードを返します。
		/// @return 現在の手番でないほうのビットボード
		[[nodiscard]]
		BitBoard getOpponentBitBoard() const
		{
			return m_opponent;
		}

		/// @brief 現在の手番の合法手のビットボードで返します。
		/// @return 現在の手番の合法手のビットボード
		[[nodiscard]]
		BitBoard getLegalBitBoard() const
		{
			constexpr int32 Shifts[8] = { 1, -1, 8, -8, 7, -7, 9, -9 };
			constexpr uint64 Masks[4] = { 0x7E7E7E7E7E7E7E7EULL, 0x00FFFFFFFFFFFF00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL };
			BitBoard result = 0ULL;

			// 縦横斜めの8方向それぞれ別に計算する
			for (int32 i = 0; i < 8; ++i)
			{
				result |= get_legal_part(Shifts[i], Masks[i / 2]);
			}

			return (result & ~(m_player | m_opponent)); // 空きマスでマスクして返す
		}

		/// @brief 現在の手番の得点を返します。
		/// @return 現在の手番の得点
		[[nodiscard]]
		int32 getPlayerScore() const
		{
			return pop_count_ull(m_player);
		}

		/// @brief 現在の手番でないほうの得点を返します。
		/// @return 現在の手番でないほうの得点
		[[nodiscard]]
		int32 getOpponentScore() const
		{
			return pop_count_ull(m_opponent);
		}

		// 盤面の石数を数える(終局した場合に使う)
		[[nodiscard]]
		int32 get_score() const
		{
			const int32 p = getPlayerScore();
			const int32 o = getOpponentScore();
			const int32 v = (64 - p - o);
			return ((p > o) ? (p - o + v) : (p - o - v));
		}

	private:

		BitBoard m_player; // その盤面から打つ手番

		BitBoard m_opponent; // その盤面で打たない手番

		// 負のシフトと正のシフトを同一に扱う関数
		static constexpr uint64 EnhancedShift(uint64 a, int32 b)
		{
			return ((b >= 0) ? (a << b) : (a >> (-b)));
		}

		// 1方向について合法手を求める
		uint64 get_legal_part(int32 shift, uint64 mask) const
		{
			uint64 o = m_opponent & mask;
			uint64 l = o & EnhancedShift(m_player, shift);

			for (int32 i = 0; i < 5; ++i)
			{
				l |= o & EnhancedShift(l, shift);
			}

			return EnhancedShift(l, shift);
		}

		// 1方向について着手ｂによって返る石を求める
		uint64 get_flip_part(int32 shift, uint64 mask, uint64 x)
		{
			uint64 o = m_opponent & mask;
			uint64 f = EnhancedShift(x, shift) & o;
			uint64 nf;
			bool flipped = false;

			for (int32 i = 0; i < 8; ++i)
			{
				nf = EnhancedShift(f, shift);

				if (nf & m_player)
				{
					flipped = true;
					break;
				}

				f |= nf & o;
			}

			if (not flipped)
			{
				f = 0ULL;
			}

			return f;
		}
	};

	// AIの根幹部分。Nega-Alpha法
	int32 nega_alpha(Board board, int32 depth, int32 alpha, int32 beta, bool passed)
	{
		if (g_abort) // 強制終了
		{
			return -Board::MaxScore;
		}

		if (depth <= 0) // 探索終了
		{
			return board.evaluate();
		}

		BitBoard legal = board.getLegalBitBoard(); // 合法手生成
		if (legal == 0ULL) // パスの場合
		{
			if (passed) // 2回パスしたら終局
			{
				return board.get_score();
			}

			board.pass();

			return -nega_alpha(board, depth, -beta, -alpha, true); // 手番を入れ替えてもう一度探索
		}

		Flip flip;

		for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)) // 合法手を走査
		{
			flip = board.getFlip(cell); // 返る石を計算

			board.move(flip); // 着手する

			alpha = std::max(alpha, -nega_alpha(board, depth - 1, -beta, -alpha, false)); // 次の手番の探索

			board.undo(flip); // 着手を取り消す

			if (beta <= alpha) // 途中で枝刈りできる場合はする
			{
				break;
			}
		}

		return alpha; // 求めた評価値を返す
	}

	// AIの計算結果
	struct AI_result
	{
		int32 pos; // 選んだ手(h8が0、g8が1、f8が2、、、、a1が63)

		int32 value; // 評価値(AI目線) 最終石差を表す。
	};

	// nega_alphaだけだと評価値を求めることしかできないので、この関数で実際に打つ手を選ぶ。
	AI_result ai(Board board, int32 depth)
	{
		AI_result res = { -1, -Board::MaxScore - 1 };
		BitBoard legal = board.getLegalBitBoard(); // 合法手生成
		int v;
		Flip flip;
		for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)) // 合法手を走査
		{
			flip = board.getFlip(cell); // 返る石を求める
			board.move(flip); // 着手
			v = -nega_alpha(board, depth - 1, -Board::MaxScore, -res.value, false); // 評価値を求める
			board.undo(flip); // 着手を取り消す
			if (res.value < v) // これまで見た評価値よりも良い評価値なら値を更新
			{
				res.pos = cell;
				res.value = v;
			}
		}
		return res;
	}

	/// @brief AI の非同期タスクを終了させます。
	/// @param task 非同期タスク
	void AbortTask(AsyncTask<OthelloAI::AI_result>& task)
	{
		if (task.isValid())
		{
			g_abort = true;

			task.get();

			g_abort = false;
		}
	}

	// GUIで使うボード
	class Game
	{
	public:

		Board board; // ビットボード

		int32 player; // Boardだけだと白番か黒番かわからないので手番情報が必要

		Game()
		{
			reset();
		}

		// ボードの初期化
		void reset()
		{
			board.reset();

			player = 0;

			m_gameOver = false;
		}

		// 着手と手番の更新
		void move(uint8 pos)
		{
			board.move(board.getFlip(pos));

			player ^= 1;

			// 合法手が無い場合は
			if (board.getLegalBitBoard() == 0ULL)
			{
				// パスして手番を変更する
				board.pass();
				player ^= 1;

				// それでも合法手が無い場合は
				if (board.getLegalBitBoard() == 0ULL)
				{
					// 終局
					m_gameOver = true;
				}
			}
		}

		/// @brief 終局しているかを返します。
		/// @return 終局している場合は true, それ以外の場合は false
		[[nodiscard]]
		bool isOver() const
		{
			return m_gameOver;
		}

		/// @brief 黒の得点を返します。
		/// @return 黒の得点
		[[nodiscard]]
		int32 getBlackScore() const
		{
			return ((player == 0) ? board.getPlayerScore() : board.getOpponentScore());
		}

		/// @brief 白の得点を返します。
		/// @return 白の得点
		[[nodiscard]]
		int32 getWhiteScore() const
		{
			return ((player == 0) ? board.getOpponentScore() : board.getPlayerScore());
		}

	private:

		// 終局しているか
		bool m_gameOver = false;
	};
}

////////////////////////////////
//
//	UI
//
////////////////////////////////

// 描画で使う定数
constexpr double BoardSize = 400;
constexpr double CellSize = (BoardSize / 8);

// ボードの描画
void DrawBoard(const OthelloAI::Game& game, const Vec2& pos, const Font& labelFont)
{
	constexpr double GridThickness = 2;
	constexpr double GridDotRadius = 5;
	constexpr double DiskRadius = 20;
	constexpr double LegalMarkRadius = 7;
	constexpr ColorF LabelColor{ 0.2 };
	constexpr ColorF GridColor{ 0.2 };
	constexpr ColorF DiskColors[2] = { Palette::Black, Palette::White };
	constexpr ColorF LegalMarkColor = Palette::Cyan;

	// 行・列ラベルを描画する
	for (int32 i = 0; i < 8; ++i)
	{
		labelFont(i + 1).draw(15, Arg::center((pos.x - 20), (pos.y + CellSize * i + CellSize / 2)), LabelColor);
		labelFont(char32(U'a' + i)).draw(15, Arg::center((pos.x + CellSize * i + CellSize / 2), (pos.y - 20 - 2)), LabelColor);
	}

	// グリッドを描画する
	for (int32 i = 0; i <= 8; ++i)
	{
		Line{ pos.x + CellSize * i, pos.y, pos.x + CellSize * i, pos.y + BoardSize }.draw(GridThickness, GridColor);
		Line{ pos.x, pos.y + CellSize * i, pos.x + BoardSize, pos.y + CellSize * i }.draw(GridThickness, GridColor);
	}

	// グリッド上の丸い模様を描画する
	{
		Circle{ (pos.x + 2 * CellSize), (pos.y + 2 * CellSize), GridDotRadius }.draw(GridColor);
		Circle{ (pos.x + 2 * CellSize), (pos.y + 6 * CellSize), GridDotRadius }.draw(GridColor);
		Circle{ (pos.x + 6 * CellSize), (pos.y + 2 * CellSize), GridDotRadius }.draw(GridColor);
		Circle{ (pos.x + 6 * CellSize), (pos.y + 6 * CellSize), GridDotRadius }.draw(GridColor);
	}

	// 石と合法手を描画する
	{
		const OthelloAI::BitBoard playerBitBoard = game.board.getPlayerBitBoard();
		const OthelloAI::BitBoard opponentBitBoard = game.board.getOpponentBitBoard();
		const OthelloAI::BitBoard legalBitBoard = game.board.getLegalBitBoard();

		for (int32 cellIndex = 0; cellIndex < 64; ++cellIndex)
		{
			const double x = pos.x + (cellIndex % 8) * CellSize + CellSize / 2;
			const double y = pos.y + (cellIndex / 8) * CellSize + CellSize / 2;

			if (OthelloAI::HasFlag(playerBitBoard, cellIndex))
			{
				Circle{ x, y, DiskRadius }.draw(DiskColors[game.player]);
			}
			else if (OthelloAI::HasFlag(opponentBitBoard, cellIndex))
			{
				Circle{ x, y, DiskRadius }.draw(DiskColors[game.player ^ 1]);
			}
			else if (OthelloAI::HasFlag(legalBitBoard, cellIndex))
			{
				Circle{ x, y, LegalMarkRadius }.draw(LegalMarkColor);
			}
		}
	}
}

// 人間の手番でマスをクリックして着手する関数
void UpdateManually(OthelloAI::Game& game, const Vec2& pos)
{
	// 現在の合法手
	OthelloAI::BitBoard legal = game.board.getLegalBitBoard();

	// 合法手を走査
	for (auto cell = OthelloAI::first_bit(&legal); legal; cell = OthelloAI::next_bit(&legal))
	{
		const int32 x = ((63 - cell) % 8); // ビットボードではh8が0だが、GUIではa1が0なので63 - cellにする
		const int32 y = ((63 - cell) / 8);

		RectF cell_rect{ (pos.x + x * CellSize), (pos.y + y * CellSize), CellSize };

		// 合法手のマスをクリックしたら着手
		if (cell_rect.leftClicked())
		{
			game.move(cell);
		}
	}
}

// AI の手番で AI が着手する関数
void UpdateAI(OthelloAI::Game& game, int32 depth, int32& value, AsyncTask<OthelloAI::AI_result>& task)
{
	// AI スレッドが未開始の場合は
	if (not task.isValid())
	{
		// AI スレッドを開始する
		task = Async(OthelloAI::ai, game.board, depth);
	}

	// AI スレッドが計算完了した場合は
	if (task.isReady())
	{
		const OthelloAI::AI_result result = task.get();

		value = result.value;

		// AI による着手
		game.move(result.pos);
	}
}

void Main()
{
	// 先読み手数（大きいと処理に時間がかかるが強くなる。1 ～ 9 が目安）
	constexpr int32 Depth = 5;

	constexpr Vec2 BoardPos{ 40, 40 };

	const Font font{ FontMethod::MSDF, 50, Typeface::Bold };

	Scene::SetBackground(Color(36, 153, 114));

	OthelloAI::Game game;

	int32 value = 0;

	int32 ai_player = 1;

	AsyncTask<OthelloAI::AI_result> task;

	while (System::Update())
	{
		////////////////////////////////
		//
		//	状態の更新
		//
		////////////////////////////////
		{
			// 終局していなければ
			if (not game.isOver())
			{
				if (game.player == ai_player)
				{
					// AI による着手
					UpdateAI(game, Depth, value, task);
				}
				else
				{
					// 人間による着手
					UpdateManually(game, BoardPos);
				}
			}
		}

		////////////////////////////////
		//
		//	描画
		//
		////////////////////////////////
		{
			// ボード
			DrawBoard(game, BoardPos, font);

			// 対局開始ボタン
			{
				if (SimpleGUI::Button(U"先手（黒）で対局開始", Vec2{ 470, 40 }))
				{
					AbortTask(task);
					game.reset();
					ai_player = 1;
				}

				if (SimpleGUI::Button(U"後手（白）で対局開始", Vec2{ 470, 80 }))
				{
					AbortTask(task);
					game.reset();
					ai_player = 0;
				}
			}

			// 手番の表示
			if (not game.isOver())
			{
				font((game.player == 0) ? U"黒番" : U"白番").draw(20, Vec2{ 470, 140 });
				font((game.player == ai_player) ? U"AI の手番" : U"あなたの手番").draw(20, Vec2{ 470, 180 });
			}
			else
			{
				font(U"終局").draw(20, Vec2{ 500, 140 });
			}
		
			// 得点の表示
			{
				Circle{ 480, 230, 12 }.draw(Palette::Black);
				Circle{ 600, 230, 12 }.draw(Palette::White);
				Line{ 540, 218, 540, 242 }.draw(2, ColorF{ 0.2 });
				font(game.getBlackScore()).draw(20, Arg::leftCenter(500, 230));
				font(game.getWhiteScore()).draw(20, Arg::rightCenter(580, 230));
			}

			font(U"評価値: {}"_fmt(value)).draw(20, Vec2{ 470, 260 });
		}
	}

	AbortTask(task);
}
