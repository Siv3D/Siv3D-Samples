# include <Siv3D.hpp>

void Main()
{
	////////////////////////////////
	// 定数(いろいろいじってみてください。)
	////////////////////////////////

	// キャンバスのサイズ
	constexpr Size canvasSize{ 600, 600 };

	// 分割数
	constexpr int32 N = 12;

	// 背景色
	constexpr Color backgroundColor = Palette::Black;

	// 一回の移動で動く距離の範囲
	constexpr double distance = 10.0;

	// 移動間隔（秒）
	constexpr double spawnTime = 1.0 / 60.0;

	////////////////////////////////
	// 初期化
	////////////////////////////////

	// ウィンドウをキャンバスのサイズに
	Window::Resize(canvasSize);

	// 蓄積された時間（秒）
	double accumulator = 0.0;

	// 書き込み用の画像
	Image image{ canvasSize, backgroundColor };

	// 画像を表示するための動的テクスチャ
	DynamicTexture texture{ image };

	// ランダムウォーカーの座標
	Vec2 walker{ 0,0 };

	while (System::Update())
	{
		// spawnTime(秒)ごとに移動
		for (accumulator += Scene::DeltaTime(); spawnTime <= accumulator; accumulator -= spawnTime)
		{

			// 移動前の座標
			const Vec2 begin = walker;

			// 移動
			walker += RandomVec2(distance);

			// 移動後の座標
			const Vec2 end = walker;

			for (auto i : step(N))
			{
				// 円座標に変換
				std::array<Circular, 2> cs = { begin, end };

				for (auto& c : cs)
				{
					// 角度をずらす
					c.theta = IsEven(i) ? (-c.theta - 2_pi / N * (i - 1)) : (c.theta + 2_pi / N * i);
				}

				// ずらした位置をもとに、画像に線を書き込む
				Line{ cs[0], cs[1] }.moveBy(Scene::Center())
					.paint(image, 1, HSV{ Scene::Time()/ spawnTime,0.7 });
			}

		}

		// Rキーを押すとリセット
		if (KeyR.down())
		{
			// 背景色で塗りつぶす
			image.fill(backgroundColor);

			// ランダムウォーカーの座標を(0,0)に戻す
			walker = { 0,0 };
		}

		// 書き込んだ画像でテクスチャを更新
		texture.fillIfNotBusy(image);

		// テクスチャを描く
		texture.draw();

	}
}
