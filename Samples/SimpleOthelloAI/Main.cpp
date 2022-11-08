# include <Siv3D.hpp> // OpenSiv3D v0.6.5

namespace OthelloAI
{
	// 計算を強制終了するときに使う
	bool global_searching = true;

	// 石を返すときに使う情報
	struct Flip
	{
		uint64 flip; // 返る石(ビットボード)
		uint_fast8_t pos; // 着手位置
	};

	// 64ビット整数の1のビットの個数を数える
	constexpr int32 pop_count_ull(uint64 x)
	{
		x = x - ((x >> 1) & 0x5555555555555555ULL);
		x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
		x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
		x = (x * 0x0101010101010101ULL) >> 56;
		return x;
	}

	// 2進数として数値を見て右端からいくつ0が連続しているか: Number of Training Zero
	inline uint_fast8_t ntz(uint64* x)
	{
		return pop_count_ull((~(*x)) & ((*x) - 1));
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

		uint64 m_player; // その盤面から打つ手番

		uint64 m_opponent; // その盤面で打たない手番

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

		// 合法手生成
		uint64 get_legal()
		{
			uint64 res = 0ULL;
			constexpr int32 shifts[8] = { 1, -1, 8, -8, 7, -7, 9, -9 };
			constexpr uint64 masks[4] = { 0x7E7E7E7E7E7E7E7EULL, 0x00FFFFFFFFFFFF00ULL, 0x007E7E7E7E7E7E00ULL, 0x007E7E7E7E7E7E00ULL };

			for (int32 i = 0; i < 8; ++i) // 縦横斜めの8方向それぞれ別に計算する
			{
				res |= get_legal_part(shifts[i], masks[i / 2]);
			}

			return res & ~(m_player | m_opponent); // 空きマスでマスクして返す
		}

		// 着手したときに返る石を求める
		Flip get_flip(uint_fast8_t pos)
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

		// 盤面の石数を数える(終局した場合に使う)
		int32 get_score()
		{
			int32 p = pop_count_ull(m_player);
			int32 o = pop_count_ull(m_opponent);
			int32 v = 64 - p - o;
			return p > o ? p - o + v : p - o - v;
		}

	private:

		// 負のシフトと正のシフトを同一に扱う関数
		static constexpr uint64 EnhancedShift(uint64 a, int32 b)
		{
			return ((b >= 0) ? (a << b) : (a >> (-b)));
		}

		// 1方向について合法手を求める
		uint64 get_legal_part(int32 shift, uint64 mask)
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
		if (!global_searching) // 強制終了
		{
			return -Board::MaxScore;
		}

		if (depth <= 0) // 探索終了
		{
			return board.evaluate();
		}

		uint64 legal = board.get_legal(); // 合法手生成
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
			flip = board.get_flip(cell); // 返る石を計算

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

		int32 val; // 評価値(AI目線) 最終石差を表す。
	};

	// nega_alphaだけだと評価値を求めることしかできないので、この関数で実際に打つ手を選ぶ。
	AI_result ai(Board board, int32 depth)
	{
		AI_result res = { -1, -Board::MaxScore - 1 };
		uint64 legal = board.get_legal(); // 合法手生成
		int v;
		Flip flip;
		for (uint_fast8_t cell = first_bit(&legal); legal; cell = next_bit(&legal)) // 合法手を走査
		{
			flip = board.get_flip(cell); // 返る石を求める
			board.move(flip); // 着手
			v = -nega_alpha(board, depth - 1, -Board::MaxScore, -res.val, false); // 評価値を求める
			board.undo(flip); // 着手を取り消す
			if (res.val < v) // これまで見た評価値よりも良い評価値なら値を更新
			{
				res.pos = cell;
				res.val = v;
			}
		}
		return res;
	}

	// AIの計算を強制終了
	void stop_calculating(std::future<OthelloAI::AI_result>* ai_future)
	{
		if (ai_future->valid())
		{
			global_searching = false;
			ai_future->get();
			global_searching = true;
		}
	}
}

// GUIで使うボード
struct Rich_board {
	OthelloAI::Board board; // ビットボード
	int player; // Boardだけだと白番か黒番かわからないので手番情報が必要
	bool game_over; // 終局しているかのフラグ

	Rich_board() {
		reset();
	}

	// ボードの初期化
	void reset() {
		board.reset();
		player = 0;
		game_over = false;
	}

	// 着手と手番変更(終局チェックも同時に行う)
	void move(uint_fast8_t pos) {
		OthelloAI::Flip flip = board.get_flip(pos);
		board.move(flip);
		player ^= 1;
		if (board.get_legal() == 0ULL) {
			board.pass();
			player ^= 1;
			if (board.get_legal() == 0ULL)
				game_over = true;
		}
	}
};

////////////////////////////////
//
//	UI
//
////////////////////////////////

// 描画で使う定数
#define BOARD_SIZE 400
#define BOARD_COORD_SIZE 20
#define DISC_SIZE 20
#define LEGAL_SIZE 7
#define STABLE_SIZE 4
#define BOARD_CELL_FRAME_WIDTH 2
#define BOARD_DOT_SIZE 5
#define BOARD_ROUND_FRAME_WIDTH 10
#define BOARD_ROUND_DIAMETER 20
#define BOARD_SY 60
constexpr int BOARD_SX = 20 + BOARD_COORD_SIZE;
constexpr int BOARD_CELL_SIZE = BOARD_SIZE / 8;

// ボードの描画
void draw_board(Font font, Font font_bold, Rich_board board) {
	// 座標の文字を描画
	String coord_x = U"abcdefgh";
	for (int i = 0; i < 8; ++i) {
		font_bold(i + 1).draw(15, Arg::center(BOARD_SX - BOARD_COORD_SIZE, BOARD_SY + BOARD_CELL_SIZE * i + BOARD_CELL_SIZE / 2), Color(51, 51, 51));
		font_bold(coord_x[i]).draw(15, Arg::center(BOARD_SX + BOARD_CELL_SIZE * i + BOARD_CELL_SIZE / 2, BOARD_SY - BOARD_COORD_SIZE - 2), Color(51, 51, 51));
	}
	// グリッドを描画
	for (int i = 0; i < 7; ++i) {
		Line(BOARD_SX + BOARD_CELL_SIZE * (i + 1), BOARD_SY, BOARD_SX + BOARD_CELL_SIZE * (i + 1), BOARD_SY + BOARD_SIZE).draw(BOARD_CELL_FRAME_WIDTH, Color(51, 51, 51));
		Line(BOARD_SX, BOARD_SY + BOARD_CELL_SIZE * (i + 1), BOARD_SX + BOARD_SIZE, BOARD_SY + BOARD_CELL_SIZE * (i + 1)).draw(BOARD_CELL_FRAME_WIDTH, Color(51, 51, 51));
	}
	//丸い模様を描画
	Circle(BOARD_SX + 2 * BOARD_CELL_SIZE, BOARD_SY + 2 * BOARD_CELL_SIZE, BOARD_DOT_SIZE).draw(Color(51, 51, 51));
	Circle(BOARD_SX + 2 * BOARD_CELL_SIZE, BOARD_SY + 6 * BOARD_CELL_SIZE, BOARD_DOT_SIZE).draw(Color(51, 51, 51));
	Circle(BOARD_SX + 6 * BOARD_CELL_SIZE, BOARD_SY + 2 * BOARD_CELL_SIZE, BOARD_DOT_SIZE).draw(Color(51, 51, 51));
	Circle(BOARD_SX + 6 * BOARD_CELL_SIZE, BOARD_SY + 6 * BOARD_CELL_SIZE, BOARD_DOT_SIZE).draw(Color(51, 51, 51));
	// 外枠を描画
	RoundRect(BOARD_SX, BOARD_SY, BOARD_SIZE, BOARD_SIZE, 20).drawFrame(0, 10, Palette::White);
	// 石と合法手を描画
	const Color colors[2] = { Palette::Black, Palette::White };
	uint64 legal = board.board.get_legal();
	for (int cell = 0; cell < 64; ++cell) {
		int x = BOARD_SX + (cell % 8) * BOARD_CELL_SIZE + BOARD_CELL_SIZE / 2;
		int y = BOARD_SY + (cell / 8) * BOARD_CELL_SIZE + BOARD_CELL_SIZE / 2;
		if (1 & (board.board.m_player >> (63 - cell))) {
			Circle(x, y, DISC_SIZE).draw(colors[board.player]);
		}
		else if (1 & (board.board.m_opponent >> (63 - cell))) {
			Circle(x, y, DISC_SIZE).draw(colors[board.player ^ 1]);
		}
		else if (1 & (legal >> (63 - cell))) {
			Circle(x, y, LEGAL_SIZE).draw(Palette::Cyan);
		}
	}
}

// 人間の手番でマスをクリックして着手する関数
void interact_move(Rich_board* board) {
	uint64 legal = board->board.get_legal(); // 合法手生成
	for (int_fast8_t cell = OthelloAI::first_bit(&legal); legal; cell = OthelloAI::next_bit(&legal)) // 合法手を走査
	{
		int x = (63 - cell) % 8; // ビットボードではh8が0だが、GUIではa1が0なので63 - cellにする
		int y = (63 - cell) / 8;
		Rect cell_rect(BOARD_SX + x * BOARD_CELL_SIZE, BOARD_SY + y * BOARD_CELL_SIZE, BOARD_CELL_SIZE, BOARD_CELL_SIZE);
		if (cell_rect.leftClicked()) { // 合法手のマスをクリックされたら着手
			board->move(cell);
		}
	}
}

// AIの手番でAIが着手する関数
void ai_move(Rich_board* board, int32 depth, int* value, std::future<OthelloAI::AI_result>* ai_future)
{
	if (ai_future->valid()) { // すでにAIが計算している場合
		if (ai_future->wait_for(std::chrono::seconds(0)) == std::future_status::ready) { // 計算が終了していたら結果を得て着手する
			OthelloAI::AI_result result = ai_future->get();
			*value = result.val;
			board->move(result.pos);
		}
	}
	else { // AIに別スレッドで計算させる
		*ai_future = std::async(std::launch::async, OthelloAI::ai, board->board, depth);
	}
}

// 画面の大きさを変えたときに綺麗に描画するために必要
double CalculateScale(const Vec2& baseSize, const Vec2& currentSize) {
	return Min((currentSize.x / baseSize.x), (currentSize.y / baseSize.y));
}

void Main()
{
	System::SetTerminationTriggers(UserAction::NoAction);
	Size window_size = Size(800, 500);
	Window::Resize(window_size);
	Window::SetStyle(WindowStyle::Sizable);
	Scene::SetResizeMode(ResizeMode::Virtual);
	Window::SetTitle(U"シンプルなオセロAI");
	Scene::SetBackground(Color(36, 153, 114));
	const Font font{ FontMethod::MSDF, 50 };
	const Font font_bold{ FontMethod::MSDF, 50, Typeface::Bold };
	Rich_board board;
	double depth = 5;
	int value = 0;
	int ai_player = 1;
	std::future<OthelloAI::AI_result> ai_future;

	while (System::Update()) {
		// 終了ボタンが押されたらAIを強制終了してからExitする
		if (System::GetUserActions() & UserAction::CloseButtonClicked) {
			stop_calculating(&ai_future);
			System::Exit();
		}

		// 画面サイズが変わったときに綺麗に描画する
		const double scale = CalculateScale(window_size, Scene::Size());
		const Transformer2D screenScaling{ Mat3x2::Scale(scale), TransformCursor::Yes };

		// ボードの描画
		draw_board(font, font_bold, board);

		// 着手する
		if (!board.game_over) { // 終局していなかったら
			if (board.player == ai_player) { // AIの手番ではAIが着手
				ai_move(&board, round(depth), &value, &ai_future);
			}
			else { // 人間の手番では人間が着手
				interact_move(&board);
			}
		}

		// 設定などのGUI
		SimpleGUI::Slider(U"先読み{}手"_fmt(round(depth)), depth, 1, 9, Vec2{ 470, 10 }, 150, 150); // 読み手数
		// 対局開始
		if (SimpleGUI::Button(U"AI先手(黒)で対局", Vec2(470, 60))) {
			stop_calculating(&ai_future);
			board.reset();
			ai_player = 0;
		}
		else if (SimpleGUI::Button(U"AI後手(白)で対局", Vec2(470, 100))) {
			stop_calculating(&ai_future);
			board.reset();
			ai_player = 1;
		}

		// 対局情報の表示
		if (!board.game_over) {
			if (board.player == 0)
				font(U"黒番").draw(20, Vec2{ 470, 140 });
			else
				font(U"白番").draw(20, Vec2{ 470, 140 });
			if (board.player == ai_player) {
				font(U"AIの手番").draw(20, Vec2{ 470, 180 });
			}
			else if (board.player == 1 - ai_player) {
				font(U"あなたの手番").draw(20, Vec2{ 470, 180 });
			}
		}
		else
			font(U"終局").draw(20, Vec2{ 500, 140 });
		int black_score = OthelloAI::pop_count_ull(board.board.m_player), white_score = OthelloAI::pop_count_ull(board.board.m_opponent);
		if (board.player == 1) // ボードには手番情報がないので、白番だったら石数を入れ替える
			std::swap(black_score, white_score);
		Circle(480, 230, 12).draw(Palette::Black);
		Circle(600, 230, 12).draw(Palette::White);
		font(black_score).draw(20, Arg::leftCenter(500, 230));
		font(white_score).draw(20, Arg::rightCenter(580, 230));
		Line(540, 218, 540, 242).draw(2, Color(51, 51, 51));
		font(U"評価値: {}"_fmt(value)).draw(20, Vec2{ 470, 260 });
	}
}
