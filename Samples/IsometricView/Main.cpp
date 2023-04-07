# include <Siv3D.hpp> // OpenSiv3D v0.6.8

// タイルの一辺の長さ（ピクセル）
inline constexpr Vec2 TileOffset{ 50, 25 };

// タイルの厚み（ピクセル）
inline constexpr int32 TileThickness = 15;

/*
	インデックスとタイルの配置の関係 (N = 4)

            (0, 0)
        (0, 1) (1, 0)
     (0, 2) (1, 1) (2, 0)
 (0, 3) (1, 2) (2, 1) (3, 0)
     (1, 3) (2, 2) (3, 1)
        (2, 3) (3, 2)
            (3, 3)
*/


/// @brief タイルのインデックスから、タイルの底辺中央の座標を計算します。
/// @param index タイルのインデックス
/// @param N マップの一辺のタイル数
/// @return タイルの底辺中央の座標
Vec2 ToTileBottomCenter(const Point& index, const int32 N)
{
	const int32 i = index.manhattanLength();
	const int32 xi = (i < (N - 1)) ? 0 : (i - (N - 1));
	const int32 yi = (i < (N - 1)) ? i : (N - 1);
	const int32 k = (index.manhattanDistanceFrom(Point{ xi, yi }) / 2);
	const double posX = ((i < (N - 1)) ? (i * -TileOffset.x) : ((i - 2 * N + 2) * TileOffset.x));
	const double posY = (i * TileOffset.y);
	return{ (posX + TileOffset.x * 2 * k), posY };
}

/// @brief タイルのインデックスから、タイルの四角形を計算します。
/// @param index タイルのインデックス
/// @param N マップの一辺のタイル数
/// @return タイルの四角形
Quad ToTile(const Point& index, const int32 N)
{
	const Vec2 bottomCenter = ToTileBottomCenter(index, N);

	return Quad{
		bottomCenter.movedBy(0, -TileThickness).movedBy(0, -TileOffset.y * 2),
		bottomCenter.movedBy(0, -TileThickness).movedBy(TileOffset.x, -TileOffset.y),
		bottomCenter.movedBy(0, -TileThickness),
		bottomCenter.movedBy(0, -TileThickness).movedBy(-TileOffset.x, -TileOffset.y)
	};
}

/// @brief 指定した列のタイルによって構成される四角形を計算します。
/// @param x 列インデックス
/// @param N マップの一辺のタイル数
/// @return 指定した列のタイルによって構成される四角形
Quad ToColumnQuad(const int32 x, const int32 N)
{
	return{
		ToTileBottomCenter(Point{ x, 0 }, N).movedBy(0, -TileThickness).movedBy(0, -TileOffset.y * 2),
		ToTileBottomCenter(Point{ x, 0 }, N).movedBy(0, -TileThickness).movedBy(TileOffset.x, -TileOffset.y),
		ToTileBottomCenter(Point{ x, (N - 1) }, N).movedBy(0, -TileThickness).movedBy(0, 0),
		ToTileBottomCenter(Point{ x, (N - 1) }, N).movedBy(0, -TileThickness).movedBy(-TileOffset.x, -TileOffset.y)
	};
}

/// @brief 指定した行のタイルによって構成される四角形を計算します。
/// @param y 行インデックス
/// @param N マップの一辺のタイル数
/// @return 指定した行のタイルによって構成される四角形
Quad ToRowQuad(const int32 y, const int32 N)
{
	return{
		ToTileBottomCenter(Point{ 0, y }, N).movedBy(0, -TileThickness).movedBy(-TileOffset.x, -TileOffset.y),
		ToTileBottomCenter(Point{ 0, y }, N).movedBy(0, -TileThickness).movedBy(0, -TileOffset.y * 2),
		ToTileBottomCenter(Point{ (N - 1), y }, N).movedBy(0, -TileThickness).movedBy(TileOffset.x, -TileOffset.y),
		ToTileBottomCenter(Point{ (N - 1), y }, N).movedBy(0, -TileThickness).movedBy(0, 0)
	};
}

/// @brief 各列のタイルによって構成される四角形の配列を作成します。
/// @param N マップの一辺のタイル数
/// @return 各列のタイルによって構成される四角形の配列
Array<Quad> MakeColumnQuads(const int32 N)
{
	Array<Quad> quads;

	for (int32 x = 0; x < N; ++x)
	{
		quads << ToColumnQuad(x, N);
	}

	return quads;
}

/// @brief 各行のタイルによって構成される四角形の配列を作成します。
/// @param N マップの一辺のタイル数
/// @return 各行のタイルによって構成される四角形の配列
Array<Quad> MakeRowQuads(const int32 N)
{
	Array<Quad> quads;

	for (int32 y = 0; y < N; ++y)
	{
		quads << ToRowQuad(y, N);
	}

	return quads;
}

/// @brief 指定した座標にあるタイルのインデックスを返します。
/// @param pos 座標
/// @param columnQuads 各列のタイルによって構成される四角形の配列
/// @param rowQuads 各行のタイルによって構成される四角形の配列
/// @return タイルのインデックス。指定した座標にタイルが無い場合は none
Optional<Point> ToIndex(const Vec2& pos, const Array<Quad>& columnQuads, const Array<Quad>& rowQuads)
{
	int32 x = -1, y = -1;

	// タイルの列インデックスを調べる
	for (int32 i = 0; i < columnQuads.size(); ++i)
	{
		if (columnQuads[i].intersects(pos))
		{
			x = i;
			break;
		}
	}

	// タイルの行インデックスを調べる
	for (int32 i = 0; i < rowQuads.size(); ++i)
	{
		if (rowQuads[i].intersects(pos))
		{
			y = i;
			break;
		}
	}

	// インデックスが -1 の場合、タイル上にはない
	if ((x == -1) || (y == -1))
	{
		return none;
	}

	return Point{ x, y };
}

void Main()
{
	// ウィンドウをリサイズする
	Window::Resize(1280, 720);

	// 背景を水色にする
	Scene::SetBackground(ColorF{ 0.8, 0.9, 1.0 });
	
	// https://kenney.nl/assets/isometric-roads
	// からファイル一式をダウンロードし、「png」フォルダを App フォルダにコピーしてください。

	// 各タイルのテクスチャ
	Array<Texture> textures;

	// png フォルダ内のファイルを列挙する
	for (const auto& filePath : FileSystem::DirectoryContents(U"png/"))
	{
		// ファイル名が conifer と tree で始まるファイル（タイルではない）は除外する
		if (const FilePath baseName = FileSystem::BaseName(filePath);
			baseName.starts_with(U"conifer") || baseName.starts_with(U"tree"))
		{
			continue;
		}

		textures << Texture{ filePath };
	}

	// 全部で 88 種類のタイルが読み込まれれば正常
	if (textures.size() != 88)
	{
		throw Error{ U"ファイルの配置が不正です。" };
	}

	// マップの一辺のタイル数
	constexpr int32 N = 8;

	// 各列の四角形
	const Array<Quad> columnQuads = MakeColumnQuads(N);

	// 各行の四角形
	const Array<Quad> rowQuads = MakeRowQuads(N);

	// タイルの種類
	Grid<int32> grid(Size{ N, N });

	// タイルメニューで選択されているタイルの種類
	int32 tileTypeSelected = 30;

	// マップ表示用の 2D カメラ
	Camera2D camera{ Vec2{ 0, 0 }, 1.0 };

	// タイルメニューの四角形
	constexpr RoundRect TileMenuRoundRect = RectF{ 20, 20, (56 * 22), (50 * 4) }.stretched(10).rounded(8);

	// マップにグリッドを表示するか
	bool showGrid = false;

	bool showIndex = false;

	while (System::Update())
	{
		// 2D カメラを更新する
		camera.update();

		// タイルメニューの四角形の上にマウスカーソルがあるか
		const bool onTileMenu = TileMenuRoundRect.mouseOver();

		{
			// 2D カメラによる座標変換を適用する
			const auto tr = camera.createTransformer();

			// 上から順にタイルを描く
			for (int32 i = 0; i < (N * 2 - 1); ++i)
			{
				// x の開始インデックス
				const int32 xi = (i < (N - 1)) ? 0 : (i - (N - 1));

				// y の開始インデックス
				const int32 yi = (i < (N - 1)) ? i : (N - 1);

				// 左から順にタイルを描く
				for (int32 k = 0; k < (N - Abs(N - i - 1)); ++k)
				{
					// タイルのインデックス
					const Point index{ (xi + k), (yi - k) };

					// そのタイルの底辺中央の座標
					const Vec2 pos = ToTileBottomCenter(index, N);

					// 底辺中央を基準にタイルを描く
					textures[grid[index]].draw(Arg::bottomCenter = pos);
				}
			}

			// マウスカーソルがタイルメニュー上に無ければ
			if (not onTileMenu)
			{
				// マウスカーソルがマップ上のどのタイルの上にあるかを取得する
				if (const auto index = ToIndex(Cursor::PosF(), columnQuads, rowQuads))
				{
					// マウスカーソルがあるタイルを強調表示する
					ToTile(*index, N).draw(ColorF{ 1.0, 0.2 });

					// マウスの左ボタンが押されていたら
					if (MouseL.pressed())
					{
						// タイルの種類を更新する
						grid[*index] = tileTypeSelected;
					}
				}
			}

			// マップ上のグリッドを表示する
			if (showGrid)
			{
				// 各列の四角形を描く
				for (const auto& columnQuad : columnQuads)
				{
					columnQuad.drawFrame(2);
				}

				// 各行の四角形を描く
				for (const auto& rowQuad : rowQuads)
				{
					rowQuad.drawFrame(2);
				}
			}

			// マップ上のタイルのインデックスを表示する
			if (showIndex)
			{
				for (int32 y = 0; y < N; ++y)
				{
					for (int32 x = 0; x < N; ++x)
					{
						const Point index{ x, y };

						const Vec2 pos = ToTileBottomCenter(index, N).movedBy(0, -TileThickness);

						PutText(U"{}"_fmt(index), pos.movedBy(0, -TileOffset.y - 3));
					}
				}
			}
		}

		// 2D カメラの UI を表示する
		camera.draw(Palette::Orange);

		// タイルメニューを表示する
		{
			// 背景
			TileMenuRoundRect.draw();

			// 各タイル
			for (int32 y = 0; y < 4; ++y)
			{
				for (int32 x = 0; x < 22; ++x)
				{
					// タイルの長方形
					const Rect rect{ (20 + x * 56), (20 + y * 50), 56, 50 };

					// タイルの種類
					const int32 tileType = (y * 22 + x);

					// 現在選択されているタイルであれば
					if (tileType == tileTypeSelected)
					{
						// 背景を灰色にする
						rect.draw(ColorF{ 0.85 });
					}

					// タイルの上にマウスカーソルがあれば
					if (rect.mouseOver())
					{
						// カーソルを手のアイコンにする
						Cursor::RequestStyle(CursorStyle::Hand);

						// 左クリックされたら		
						if (MouseL.down())
						{
							// 選択しているタイルの種類を更新する
							tileTypeSelected = tileType;
						}
					}

					// タイルを表示する
					textures[tileType].scaled(0.5).drawAt(rect.center());
				}
			}
		}

		// マップ上のグリッドを表示するかのチェックボックス
		SimpleGUI::CheckBox(showGrid, U"Show grid", Vec2{ 20, 240 }, 160);

		// マップ上のインデックスを表示するかのチェックボックス
		SimpleGUI::CheckBox(showIndex, U"Show index", Vec2{ 20, 280 }, 160);
	}
}
