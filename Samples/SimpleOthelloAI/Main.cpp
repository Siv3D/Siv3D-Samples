# include <Siv3D.hpp> // OpenSiv3D v0.6.5

namespace OthelloAI
{
	// ビットボード
	using BitBoard = uint64;

	/// @brief ビットボード上のインデックス
	/// @remark A1 が 63, B1 が 62, C1 が 61, ... H8 が 0
	using BitBoardIndex = uint8;

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

	// 着手の情報
	struct Record
	{
		// 着手位置
		BitBoardIndex pos;

		// 返る石
		BitBoard flip;
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

		/// @brief 局面を初期化します。
		void reset()
		{
			m_player = 0x0000000810000000ULL;
			m_opponent = 0x0000001008000000ULL;
		}

		/// @brief 着手します
		/// @param record 着手情報
		void move(Record record)
		{
			m_player ^= record.flip;
			m_opponent ^= record.flip;
			m_player ^= (1ULL << record.pos);
			std::swap(m_player, m_opponent);
		}

		/// @brief 着手を取り消します。
		/// @param record 取り消す着手情報
		void undo(Record record)
		{
			std::swap(m_player, m_opponent);
			m_player ^= (1ULL << record.pos);
			m_player ^= record.flip;
			m_opponent ^= record.flip;
		}

		/// @brief ある着手を行ったときの着手情報を返します。
		/// @param move 着手位置
		/// @return 着手情報
		Record makeRecord(BitBoardIndex pos) const
		{
			constexpr int32 Shifts[8] = { 1, -1, 8, -8, 7, -7, 9, -9 };
			constexpr uint64 Masks[4] = { 0x7E7E7E7E7E7E7E7EULL, 0x00FFFFFFFFFFFF00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL };
			const uint64 x = (1ULL << pos);
			Record record{ .pos = pos, .flip = 0ULL };

			// 縦横斜めの8方向それぞれ別に計算する
			for (int32 i = 0; i < 8; ++i)
			{
				record.flip |= GetFlipPart(m_player, m_opponent, Shifts[i], Masks[i / 2], x);
			}

			return record;
		}

		// パス
		void pass()
		{
			std::swap(m_player, m_opponent);
		}

		// 評価関数(終局していない場合に使う。マスの重みを使った評価で最終石差を推測する)
		int32 evaluate() const
		{
			constexpr int32 CellWeightScores[10] = { 2714, 147, 69, -18, -577, -186, -153, -379, -122, -169 };
			constexpr uint64 CellWeightMasks[10] = { 0x8100000000000081ULL, 0x4281000000008142ULL, 0x2400810000810024ULL, 0x1800008181000018ULL, 0x0042000000004200ULL,
				0x0024420000422400ULL, 0x0018004242001800ULL, 0x0000240000240000ULL, 0x0000182424180000ULL, 0x0000001818000000ULL };
			int32 result = 0;

			for (int32 i = 0; i < 10; ++i) // 盤面を10種類のマスに分けてそれぞれのマスに重みをつけたので、1種類ずつ計算
			{
				result += CellWeightScores[i] * (pop_count_ull(m_player & CellWeightMasks[i]) - pop_count_ull(m_opponent & CellWeightMasks[i]));
			}

			result += (result > 0 ? 128 : (result < 0 ? -128 : 0));
			result /= 256; // 最終石差の 256 倍を学習データにしたので、256 で割って実際の最終石差の情報にする
			return Max(-MaxScore, Min(MaxScore, result)); // -64 から +64 までの範囲に収める
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
				result |= GetLegalPart(m_player, m_opponent, Shifts[i], Masks[i / 2]);
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

		[[nodiscard]]
		int32 getScore() const
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

		// 1 方向について合法手を求める
		static constexpr uint64 GetLegalPart(BitBoard player, BitBoard opponent, int32 shift, uint64 mask)
		{
			uint64 o = (opponent & mask);
			uint64 l = o & EnhancedShift(player, shift);

			for (int32 i = 0; i < 5; ++i)
			{
				l |= o & EnhancedShift(l, shift);
			}

			return EnhancedShift(l, shift);
		}

		// 1 方向について着手 ｂ によって返る石を求める
		static constexpr uint64 GetFlipPart(BitBoard player, BitBoard opponent, int32 shift, uint64 mask, uint64 x)
		{
			uint64 o = (opponent & mask);
			uint64 f = (EnhancedShift(x, shift) & o);
			uint64 nf = 0ULL;
			bool flipped = false;

			for (int32 i = 0; i < 8; ++i)
			{
				nf = EnhancedShift(f, shift);

				if (nf & player)
				{
					flipped = true;
					break;
				}

				f |= (nf & o);
			}

			if (not flipped)
			{
				f = 0ULL;
			}

			return f;
		}
	};

	// AI の根幹部分。Nega-Alpha 法
	int32 NegaAlpha(Board board, int32 depth, int32 alpha, int32 beta, bool passed)
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
				return board.getScore();
			}

			board.pass();

			return -NegaAlpha(board, depth, -beta, -alpha, true); // 手番を入れ替えてもう一度探索
		}

		Record record;

		for (BitBoardIndex cell = first_bit(&legal); legal; cell = next_bit(&legal)) // 合法手を走査
		{
			record = board.makeRecord(cell); // 返る石を計算

			board.move(record); // 着手する

			alpha = Max(alpha, -NegaAlpha(board, depth - 1, -beta, -alpha, false)); // 次の手番の探索

			board.undo(record); // 着手を取り消す

			if (beta <= alpha) // 途中で枝刈りできる場合はする
			{
				break;
			}
		}

		return alpha; // 求めた評価値を返す
	}

	/// @brief AI の計算結果
	struct AI_Result
	{
		/// @brief 選んだ手
		BitBoardIndex pos;

		/// @brief AI 目線での評価値（最終石差）
		int32 value;
	};

	// nega_alphaだけだと評価値を求めることしかできないので、この関数で実際に打つ手を選ぶ。
	AI_Result AITask(Board board, int32 depth)
	{
		AI_Result result = { 0, (-Board::MaxScore - 1) };

		BitBoard legal = board.getLegalBitBoard(); // 合法手生成

		int32 value = 0;

		Record record;

		// 各合法手について
		for (BitBoardIndex pos = first_bit(&legal); legal; pos = next_bit(&legal))
		{
			record = board.makeRecord(pos); // 返る石を求める

			board.move(record); // 着手

			value = -NegaAlpha(board, depth - 1, -Board::MaxScore, -result.value, false); // 評価値を求める

			board.undo(record); // 着手を取り消す

			if (result.value < value) // これまで見た評価値よりも良い評価値なら値を更新
			{
				result = { pos, value };
			}
		}

		return result;
	}

	/// @brief AI の非同期タスクを終了させます。
	/// @param task 非同期タスク
	void AbortTask(AsyncTask<OthelloAI::AI_Result>& task)
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

		Game()
		{
			reset();
		}

		[[nodiscard]]
		const Board& getBoard() const
		{
			return m_board;
		}

		// ボードの初期化
		void reset()
		{
			m_board.reset();

			m_activePlayer = 0;

			m_gameOver = false;
		}

		// 着手と手番の更新
		void move(BitBoardIndex pos)
		{
			m_board.move(m_board.makeRecord(pos));

			m_activePlayer ^= 1;

			// 合法手が無い場合は
			if (m_board.getLegalBitBoard() == 0ULL)
			{
				// パスして手番を変更する
				m_board.pass();

				m_activePlayer ^= 1;

				// それでも合法手が無い場合は
				if (m_board.getLegalBitBoard() == 0ULL)
				{
					// 終局
					m_gameOver = true;
				}
			}
		}

		/// @brief 現在アクティブな手番を返します。
		/// @return 現在アクティブな手番（0: 黒, 1: 白）
		[[nodiscard]]
		int32 getActivePlayer() const
		{
			return m_activePlayer;
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
			return ((m_activePlayer == 0) ? m_board.getPlayerScore() : m_board.getOpponentScore());
		}

		/// @brief 白の得点を返します。
		/// @return 白の得点
		[[nodiscard]]
		int32 getWhiteScore() const
		{
			return ((m_activePlayer == 0) ? m_board.getOpponentScore() : m_board.getPlayerScore());
		}

	private:

		// ビットボード
		Board m_board;

		// 現在アクティブな手番（0: 黒, 1: 白）
		int32 m_activePlayer = 0;

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
		const OthelloAI::BitBoard playerBitBoard = game.getBoard().getPlayerBitBoard();
		const OthelloAI::BitBoard opponentBitBoard = game.getBoard().getOpponentBitBoard();
		const OthelloAI::BitBoard legalBitBoard = game.getBoard().getLegalBitBoard();

		for (int32 cellIndex = 0; cellIndex < 64; ++cellIndex)
		{
			const double x = pos.x + (cellIndex % 8) * CellSize + CellSize / 2;
			const double y = pos.y + (cellIndex / 8) * CellSize + CellSize / 2;

			if (OthelloAI::HasFlag(playerBitBoard, cellIndex))
			{
				Circle{ x, y, DiskRadius }.draw(DiskColors[game.getActivePlayer()]);
			}
			else if (OthelloAI::HasFlag(opponentBitBoard, cellIndex))
			{
				Circle{ x, y, DiskRadius }.draw(DiskColors[game.getActivePlayer() ^ 1]);
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
	OthelloAI::BitBoard legal = game.getBoard().getLegalBitBoard();

	// 合法手を走査
	for (OthelloAI::BitBoardIndex cell = OthelloAI::first_bit(&legal); legal; cell = OthelloAI::next_bit(&legal))
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
void UpdateAI(OthelloAI::Game& game, int32 depth, int32& value, AsyncTask<OthelloAI::AI_Result>& task)
{
	// AI スレッドが未開始の場合は
	if (not task.isValid())
	{
		// AI スレッドを開始する
		task = Async(OthelloAI::AITask, game.getBoard(), depth);
	}

	// AI スレッドが計算完了した場合は
	if (task.isReady())
	{
		const OthelloAI::AI_Result result = task.get();

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

	// 人間のプレイヤーのインデックス (0 の場合先手、1 の場合後手）
	int32 humanPlayer = 0;

	AsyncTask<OthelloAI::AI_Result> task;

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
				if (game.getActivePlayer() == humanPlayer)
				{
					// 人間による着手
					UpdateManually(game, BoardPos);
				}
				else
				{
					// AI による着手
					UpdateAI(game, Depth, value, task);
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
					humanPlayer = 0;
				}

				if (SimpleGUI::Button(U"後手（白）で対局開始", Vec2{ 470, 80 }))
				{
					AbortTask(task);
					game.reset();
					humanPlayer = 1;
				}
			}

			// 手番の表示
			if (not game.isOver())
			{
				font((game.getActivePlayer() == 0) ? U"黒番" : U"白番").draw(20, Vec2{ 470, 140 });
				font((game.getActivePlayer() == humanPlayer) ? U"あなたの手番" : U"AI の手番").draw(20, Vec2{ 470, 180 });
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
