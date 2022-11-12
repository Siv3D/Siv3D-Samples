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

	/// @brief 色
	enum class Color
	{
		Black,

		White
	};

	/// @brief 反対の色を返します。
	/// @param c 色
	/// @return 反対の色
	[[nodiscard]]
	constexpr Color operator ~(Color c)
	{
		return ((c == Color::Black) ? Color::White : Color::Black);
	}

	/// @brief セルのインデックスをビットボード上のインデックスに変換します。
	/// @param i セルのインデックス
	/// @return ビットボード上のインデックス
	[[nodiscard]]
	constexpr BitBoardIndex ToBitBoardIndex(CellIndex i)
	{
		return static_cast<BitBoardIndex>(63 - i);
	}

	/// @brief ビットボード上のインデックスをセルのインデックスに変換します。
	/// @param i ビットボード上のインデックス
	/// @return セルのインデックス
	[[nodiscard]]
	constexpr CellIndex ToCellIndex(BitBoardIndex i)
	{
		return static_cast<CellIndex>(63 - i);
	}

	/// @brief ビットボードを bool 型の配列に変換します。
	/// @param bitBoard ビットボード
	/// @return bool 型の配列
	[[nodiscard]]
	constexpr std::array<bool, 64> ToArray(BitBoard bitBoard)
	{
		std::array<bool, 64> results{};

		for (CellIndex i = 0; i < 64; ++i)
		{
			results[i] = static_cast<bool>(1 & (bitBoard >> (63 - i)));
		}

		return results;
	}

	/// @brief 着手の情報
	struct Move
	{
		/// @brief 着手位置
		BitBoardIndex pos;

		/// @brief 返る石
		BitBoard flip;

		/// @brief 着手位置をセルのインデックスで返します。
		/// @return 着手位置（セルのインデックス）
		CellIndex asCellIndex() const
		{
			return ToCellIndex(pos);
		}

		/// @brief 着手位置を符号で返します。
		/// @return 着手位置の符号
		String asLabel() const
		{
			return{ char32('h' - (pos % 8)), char32('8' - (pos / 8)) };
		}
	};

	/// @brief ビットボード
	class Board
	{
	public:

		/// @brief スコアの絶対値の最大値
		static constexpr int32 MaxScore = 64;

		/// @brief 局面を初期化します。
		void reset()
		{
			m_player = 0x0000000810000000ULL;
			m_opponent = 0x0000001008000000ULL;
		}

		/// @brief 着手します。
		/// @param move 着手情報
		void move(Move move)
		{
			m_player ^= move.flip;
			m_opponent ^= move.flip;
			m_player ^= (1ULL << move.pos);
			std::swap(m_player, m_opponent);
		}

		/// @brief 着手を取り消します。
		/// @param move 取り消す着手情報
		void undo(Move move)
		{
			std::swap(m_player, m_opponent);
			m_player ^= (1ULL << move.pos);
			m_player ^= move.flip;
			m_opponent ^= move.flip;
		}

		/// @brief ある着手を行った場合の着手情報を返します。
		/// @param move 着手位置
		/// @return 着手情報
		Move makeMove(BitBoardIndex pos) const
		{
			constexpr int32 Shifts[8] = { 1, -1, 8, -8, 7, -7, 9, -9 };
			constexpr uint64 Masks[4] = { 0x7E7E7E7E7E7E7E7EULL, 0x00FFFFFFFFFFFF00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL };
			const uint64 x = (1ULL << pos);
			Move move{ .pos = pos, .flip = 0ULL };

			// 縦横斜めの8方向それぞれ別に計算する
			for (int32 i = 0; i < 8; ++i)
			{
				move.flip |= GetFlipPart(m_player, m_opponent, Shifts[i], Masks[i / 2], x);
			}

			return move;
		}

		/// @brief 手番を入れ替えます。
		void pass()
		{
			std::swap(m_player, m_opponent);
		}

		/// @brief マスの重みを使った評価で最終石差を推測します（終局していないときに使います）。
		/// @return 評価値
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

		/// @brief 64 ビット整数の 1 のビットの個数を数えます。
		/// @param x 整数
		/// @return 1 のビットの個数
		static constexpr int32 pop_count_ull(uint64 x)
		{
			x = x - ((x >> 1) & 0x5555555555555555ULL);
			x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
			x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
			x = (x * 0x0101010101010101ULL) >> 56;
			return static_cast<int32>(x);
		}

	private:

		// その盤面から打つ手番
		BitBoard m_player = 0;

		// その盤面で打たない手番
		BitBoard m_opponent = 0;

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

	/// @brief ゲーム情報
	class Game
	{
	public:

		/// @brief AI の計算結果
		struct AI_Result
		{
			/// @brief 選んだ手
			BitBoardIndex pos;

			/// @brief AI 目線での評価値（最終石差）
			int32 value;
		};

		Game()
		{
			reset();
		}

		~Game()
		{
			AbortTask(m_task);
		}

		void setAIDepth(int32 depth)
		{
			m_depth = depth;
		}

		/// @brief ゲームを初期化します。
		void reset()
		{
			AbortTask(m_task);

			m_board.reset();

			m_activeColor = OthelloAI::Color::Black;

			m_gameOver = false;

			m_history.clear();
		}

		/// @brief 着手します。
		/// @param pos 着手位置
		/// @return 着手情報
		Move move(BitBoardIndex pos)
		{
			const Move move = m_board.makeMove(pos);

			m_history.emplace_back(m_activeColor, move);

			m_board.move(move);

			m_activeColor = ~m_activeColor;

			// 合法手が無い場合は
			if (m_board.getLegalBitBoard() == 0ULL)
			{
				// パスして手番を変更する
				m_board.pass();

				m_activeColor = ~m_activeColor;

				// それでも合法手が無い場合は
				if (m_board.getLegalBitBoard() == 0ULL)
				{
					// 終局
					m_gameOver = true;
				}
			}

			return move;
		}

		/// @brief AI に現在の手番で最適な着手位置を非同期で計算してもらいます。
		/// @return 計算結果。計算途中の場合は none
		[[nodiscard]]
		Optional<AI_Result> calculateAsync() const
		{
			// AI スレッドが未開始の場合は
			if (not m_task.isValid())
			{
				// AI スレッドを開始する
				m_task = Async(AITask, m_board, m_depth);
			}

			// AI スレッドが計算完了した場合は
			if (m_task.isReady())
			{
				return m_task.get();
			}

			return none;
		}

		/// @brief AI に現在の手番で最適な着手位置を計算してもらいます。
		/// @return 計算結果
		AI_Result calculate() const
		{
			return AITask(m_board, m_depth);
		}

		/// @brief 黒の石の配置を返します。
		/// @return 黒の石の配置
		[[nodiscard]]
		std::array<bool, 64> getBlackDisks() const
		{
			return ToArray((m_activeColor == OthelloAI::Color::Black) ? m_board.getPlayerBitBoard() : m_board.getOpponentBitBoard());
		}

		/// @brief 白の石の配置を返します。
		/// @return 白の石の配置
		[[nodiscard]]
		std::array<bool, 64> getWhiteDisks() const
		{
			return ToArray((m_activeColor == OthelloAI::Color::Black) ? m_board.getOpponentBitBoard() : m_board.getPlayerBitBoard());
		}

		/// @brief 合法手の配置を返します。
		/// @return 合法手の配置
		[[nodiscard]]
		std::array<bool, 64> getLegals() const
		{
			return ToArray(m_board.getLegalBitBoard());
		}

		/// @brief 現在アクティブな色を返します。
		/// @return 現在アクティブな色
		[[nodiscard]]
		OthelloAI::Color getActiveColor() const
		{
			return m_activeColor;
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
			return ((m_activeColor == OthelloAI::Color::Black) ? m_board.getPlayerScore() : m_board.getOpponentScore());
		}

		/// @brief 白の得点を返します。
		/// @return 白の得点
		[[nodiscard]]
		int32 getWhiteScore() const
		{
			return ((m_activeColor == OthelloAI::Color::Black) ? m_board.getOpponentScore() : m_board.getPlayerScore());
		}

		/// @brief 着手の履歴を返します。
		/// @return 着手の履歴
		[[nodiscard]]
		const Array<std::pair<OthelloAI::Color, OthelloAI::Move>>& getHistory() const
		{
			return m_history;
		}

		/// @brief ビットボードを返します。
		/// @return ビットボード
		[[nodiscard]]
		const Board& getBoard() const
		{
			return m_board;
		}

	private:

		// ビットボード
		Board m_board;

		// 現在アクティブな色
		OthelloAI::Color m_activeColor = OthelloAI::Color::Black;

		// 着手履歴
		Array<std::pair<OthelloAI::Color, OthelloAI::Move>> m_history;

		// 終局しているか
		bool m_gameOver = false;

		// 先読みの手数
		int32 m_depth = 5;

		// AI の非同期タスク
		mutable AsyncTask<AI_Result> m_task;

		// AI 非同期タスクの中断フラグ
		inline static std::atomic<bool> m_abort = false;

		// 2 進数として数値を見て右端からいくつ 0 が連続しているか: Number of Training Zero
		static uint_fast8_t ntz(uint64* x)
		{
			return static_cast<uint_fast8_t>(Board::pop_count_ull((~(*x)) & ((*x) - 1)));
		}

		// 立っているビットを走査するときに for 文で使うと便利
		static uint_fast8_t first_bit(uint64* x)
		{
			return ntz(x);
		}

		// 立っているビットを走査するときに for 文で使うと便利
		static uint_fast8_t next_bit(uint64* x)
		{
			*x &= *x - 1; // 最右の立っているビットをオフにする
			return ntz(x);
		}

		// AI の根幹部分。Nega-Alpha 法
		static int32 NegaAlpha(Board board, int32 depth, int32 alpha, int32 beta, bool passed)
		{
			if (m_abort) // 強制終了
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

			Move move;

			for (BitBoardIndex cell = first_bit(&legal); legal; cell = next_bit(&legal)) // 合法手を走査
			{
				move = board.makeMove(cell); // 返る石を計算

				board.move(move); // 着手する

				alpha = Max(alpha, -NegaAlpha(board, depth - 1, -beta, -alpha, false)); // 次の手番の探索

				board.undo(move); // 着手を取り消す

				if (beta <= alpha) // 途中で枝刈りできる場合はする
				{
					break;
				}
			}

			return alpha; // 求めた評価値を返す
		}

		// NegaAlpha は評価値を求めることしかできないので、この関数で実際に打つ手を選ぶ。
		static AI_Result AITask(Board board, int32 depth)
		{
			AI_Result result = { 0, (-Board::MaxScore - 1) };

			BitBoard legal = board.getLegalBitBoard(); // 合法手生成

			int32 value = 0;

			Move move;

			// 各合法手について
			for (BitBoardIndex pos = first_bit(&legal); legal; pos = next_bit(&legal))
			{
				move = board.makeMove(pos); // 返る石を求める

				board.move(move); // 着手

				value = -NegaAlpha(board, depth - 1, -Board::MaxScore, -result.value, false); // 評価値を求める

				board.undo(move); // 着手を取り消す

				if (result.value < value) // これまで見た評価値よりも良い評価値なら値を更新
				{
					result = { pos, value };
				}
			}

			return result;
		}

		// 非同期タスクを中断する
		static void AbortTask(AsyncTask<AI_Result>& task)
		{
			if (task.isValid())
			{
				m_abort = true;

				task.get();

				m_abort = false;
			}
		}
	};
}

////////////////////////////////
//
//	UI
//
////////////////////////////////

/// @brief ボードのサイズ
constexpr double BoardSize = 400;

/// @brief セルの大きさ
constexpr double CellSize = (BoardSize / 8);

/// @brief 黒石の色
constexpr ColorF BlackDiskColor{ 0.11 };

/// @brief 白石の色
constexpr ColorF WhiteDiskColor{ 0.98 };

/// @brief セルのインデックスを座標に変換します。
/// @param i セルのインデックス
/// @return セルの座標
[[nodiscard]]
constexpr Vec2 ToVec2(OthelloAI::CellIndex i)
{
	return (Vec2{ (i % 8), (i / 8) } * CellSize + CellSize * Vec2{ 0.5, 0.5 });
}

/// @brief 盤面を描画します。
/// @param game ゲーム
/// @param pos ボードの左上の位置
/// @param labelFont ラベル用フォント
/// @param t アニメーション [0.0, 1.0]
void DrawBoard(const OthelloAI::Game& game, const Vec2& pos, const Font& labelFont, double t)
{
	constexpr double GridThickness = 2;
	constexpr double GridDotRadius = (CellSize * 0.1);
	constexpr double DiskRadius = (CellSize * 0.4);
	constexpr ColorF GridColor{ 0.2 };
	constexpr ColorF DiskShadowColor{ 0.0, 0.5 };

	// 行・列ラベルを描画する
	for (int32 i = 0; i < 8; ++i)
	{
		labelFont(i + 1).draw(15, Arg::center((pos.x - 20), (pos.y + CellSize * i + CellSize / 2)), GridColor);
		labelFont(char32(U'a' + i)).draw(15, Arg::center((pos.x + CellSize * i + CellSize / 2), (pos.y - 20 - 2)), GridColor);
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

	// 石を描画する
	{
		const std::array<bool, 64> balckDisks = game.getBlackDisks();
		const std::array<bool, 64> whiteDisks = game.getWhiteDisks();

		std::array<bool, 64> flips{};
		if (game.getHistory())
		{
			flips = OthelloAI::ToArray(game.getHistory().back().second.flip);
		}

		t = EaseInOutCirc(t);

		for (OthelloAI::CellIndex i = 0; i < 64; ++i)
		{
			const Vec2 center = pos + ToVec2(i);
			const Circle disk{ center, DiskRadius };

			if (flips[i] && (t < 1.0))
			{
				const Transformer2D tr{ Mat3x2::Scale((t < 0.5) ? (0.5 - t) * 2 : (t - 0.5) * 2, 1.0, center) };

				disk.drawShadow(Vec2{ 0, 2 }, 7, 2, DiskShadowColor);

				if (balckDisks[i])
				{
					if (t < 0.5)
					{
						disk.draw(WhiteDiskColor);
					}
					else
					{
						disk.draw(BlackDiskColor);
					}
				}
				else if (whiteDisks[i])
				{
					if (t < 0.5)
					{
						disk.draw(BlackDiskColor);
					}
					else
					{
						disk.draw(WhiteDiskColor);
					}
				}
			}
			else
			{
				if (balckDisks[i])
				{
					disk.drawShadow(Vec2{ 0, 2 }, 7, 2, DiskShadowColor).draw(BlackDiskColor);
				}
				else if (whiteDisks[i])
				{
					disk.drawShadow(Vec2{ 0, 2 }, 7, 2, DiskShadowColor).draw(WhiteDiskColor);
				}
			}
		}
	}
}

/// @brief 人間の手番でセルをクリックして着手します。
/// @param game ゲーム
/// @param pos ボードの左上の位置
/// @return 着手した場合はビットボード上のインデックス、それ以外の場合は none
Optional<OthelloAI::BitBoardIndex> UpdateManually(OthelloAI::Game& game, const Vec2& pos)
{
	// 現在の合法手
	const std::array<bool, 64> legals = game.getLegals();

	for (OthelloAI::CellIndex i = 0; i < 64; ++i)
	{
		// 合法手でなければスキップ
		if (not legals[i])
		{
			continue;
		}

		const RectF cell{ Arg::center = (pos + ToVec2(i)), CellSize };

		cell.drawFrame(CellSize * 0.15, 0, ColorF{ 1.0, 0.4 });

		// 合法手をマウスオーバーしていたら
		if (cell.mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hand);

			cell.draw(ColorF{ 1.0, 0.5 });

			// その合法手で返すことのできる石
			const std::array<bool, 64> flips = OthelloAI::ToArray(game.getBoard().makeMove(OthelloAI::ToBitBoardIndex(i)).flip);

			for (OthelloAI::CellIndex k = 0; k < 64; ++k)
			{
				if (flips[k])
				{
					RectF{ Arg::center = (pos + ToVec2(k)), CellSize }
						.draw(ColorF{ Palette::Orange, 0.6 });
				}
			}

			if (cell.leftClicked())
			{
				return OthelloAI::ToBitBoardIndex(i);
			}
		}
	}

	return none;
}

void Main()
{
	Scene::SetBackground(ColorF{ 0.15, 0.6, 0.45 });

	constexpr Vec2 BoardOffset{ 40, 40 };

	const Font font{ FontMethod::MSDF, 48, Typeface::Bold };

	// 手番開始時のクールタイム
	constexpr Duration CoolTime = 0.5s;

	// ゲームの情報
	OthelloAI::Game game;

	// AI の先読み手数（先読み手数が大きいと強くなるが、計算時間が長くなる。1 ～ 9 が目安）
	game.setAIDepth(5);

	// AI 視点での評価値
	int32 value = 0;

	// 人間プレイヤーの色
	OthelloAI::Color humanColor = OthelloAI::Color::Black;

	// 着手からの経過時間測定
	Stopwatch stopwatch{ StartImmediately::Yes };

	while (System::Update())
	{
		////////////////////////////////
		//
		//	状態の更新
		//
		////////////////////////////////
		{
			// 終局していなければ
			if (not game.isOver() && (CoolTime <= stopwatch))
			{
				if (game.getActiveColor() == humanColor) // 人間の手番
				{
					// 人間による着手
					if (const auto result = UpdateManually(game, BoardOffset))
					{
						const auto record = game.move(*result);
						stopwatch.restart();
					}
				}
				else // AI の手番
				{
					// AI による着手
				# if SIV3D_PLATFORM(WEB)

					const auto result = game.calculate();
					const auto record = game.move(result.pos);
					value = result.value;
					stopwatch.restart();

				# else

					// 非同期で計算
					if (const auto result = game.calculateAsync())
					{
						const auto record = game.move(result->pos);
						value = result->value;
						stopwatch.restart();
					}

				# endif
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
			DrawBoard(game, BoardOffset, font, Min(1.0, (stopwatch.elapsed() / (CoolTime * 0.6))));

			// 対局開始ボタン
			{
				Optional<OthelloAI::Color> reset;

				if (SimpleGUI::Button(U"\U000F012F 先手 (黒) で対局開始", Vec2{ 470, 40 }))
				{
					reset = OthelloAI::Color::Black;
				}

				if (SimpleGUI::Button(U"\U000F0130 後手 (白) で対局開始", Vec2{ 470, 80 }))
				{
					reset = OthelloAI::Color::White;
				}

				if (reset)
				{
					game.reset();
					value = 0;
					humanColor = *reset;
				}
			}

			// 手番の表示
			if (not game.isOver())
			{
				font(U"{}番（{}の手番）"_fmt(
					((game.getActiveColor() == OthelloAI::Color::Black) ? U'黒' : U'白'),
					((game.getActiveColor() == humanColor) ? U"あなた" : U"AI ")
					)).draw(20, Vec2{ 470, 140 });
			}
			else
			{
				font(U"終局").draw(20, Vec2{ 470, 140 });
			}
		
			// 得点の表示
			{
				Circle{ 480, 190, 12 }.draw(BlackDiskColor);
				Circle{ 600, 190, 12 }.draw(WhiteDiskColor);
				Line{ 540, 178, 540, 202 }.draw(2, ColorF{ 0.2 });
				font(game.getBlackScore()).draw(20, Arg::leftCenter(500, 190));
				font(game.getWhiteScore()).draw(20, Arg::rightCenter(580, 190));
			}

			font(U"AI 視点の評価値: {}"_fmt(value)).draw(20, Vec2{ 470, 220 });
		}
	}
}
