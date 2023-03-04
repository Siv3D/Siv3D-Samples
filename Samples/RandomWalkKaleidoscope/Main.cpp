# include <Siv3D.hpp>

void Main()
{
	// キャンバスのサイズ
	constexpr Size CanvasSize{ 600, 600 };

	// 分割数
	int32 N = 12;

	// 背景色
	constexpr Color BackgroundColor = Palette::Black;

	// 一回の移動の最大距離
	constexpr double MaxWalkDistance = 10.0;

	// 移動間隔（秒）
	constexpr double UpdateInterval = (1.0 / 60.0);

	// ウィンドウをキャンバスのサイズにする
	Window::Resize(CanvasSize);

	// 書き込み用の画像
	Image image{ CanvasSize, BackgroundColor };

	// 画像を表示するための動的テクスチャ
	DynamicTexture texture{ image };

	// ランダムウォーカーの座標
	Vec2 walker{ 0,0 };

	// 蓄積された時間（秒）
	double accumulatedTime = 0.0;

	while (System::Update())
	{
		accumulatedTime += Scene::DeltaTime();

		// UpdateInterval が経過するたびに移動
		while (UpdateInterval <= accumulatedTime)
		{
			// 移動前の座標
			const Vec2 from = walker;

			// 座標をランダムに移動させる
			walker += (RandomVec2() * Random(MaxWalkDistance));

			// 移動後の座標
			const Vec2 to = walker;

			// 線の色
			const HSV color{ (0.5 * Scene::Time() / UpdateInterval), 0.7, 0.8 };

			for (int32 i = 0; i < N; ++i)
			{
				// 円座標に変換する
				std::array<Circular, 2> cs = { from, to };

				for (auto& c : cs)
				{
					// 角度をずらす
					c.theta = IsEven(i) ? (-c.theta - 2_pi / N * (i - 1)) : (c.theta + 2_pi / N * i);
				}

				// ずらした位置をもとに, 画像に線を書き込む
				Line{ cs[0], cs[1] }.moveBy(CanvasSize / 2)
					.overwrite(image, 1, color);
			}

			accumulatedTime -= UpdateInterval;
		}

		// 左クリックでリセットする
		if (MouseL.down())
		{
			// 背景色で塗りつぶす
			image.fill(BackgroundColor);

			// ランダムウォーカーの座標を (0,0) に戻す
			walker.clear();

			// 分割数をランダムな 4～24 の間の偶数に変更する
			N = (Random(2, 12) * 2);
		}

		// 書き込んだ画像でテクスチャを更新する
		texture.fillIfNotBusy(image);

		// テクスチャを描く
		texture.draw();
	}
}
