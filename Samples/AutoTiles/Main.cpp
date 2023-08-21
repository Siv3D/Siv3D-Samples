# include <Siv3D.hpp> // OpenSiv3D v0.6.10

class AutoTile
{
public:

	AutoTile() = default;

	/// @brief オートタイルを作成します。
	/// @param image オートタイルの基本画像（1x5 タイル、または 8x6 タイル）
	SIV3D_NODISCARD_CXX20
	explicit AutoTile(const Image& image)
	{
		if (image.height() == (image.width() * 5))
		{
			*this = CreateFromBaseImage(image);
		}
		else if ((image.width() * 6) == (image.height() * 8))
		{
			*this = CreateFromTiles(image);
		}
	}

	/// @brief タイルのサイズ（ピクセル）を返します。
	/// @return タイルのサイズ（ピクセル）
	[[nodiscard]]
	int32 getTileSize() const noexcept
	{
		return m_tileSize;
	}

	/// @brief オートタイルを返します。
	/// @param bits 周囲のオートタイルの状況を表す 8 ビットの値
	/// @return オートタイル
	[[nodiscard]]
	TextureRegion getTile(uint8 bits) const
	{
		const int32 tileIndex = GetTileIndex(bits);
		const int32 x = ((tileIndex % 8) * m_tileSize);
		const int32 y = ((tileIndex / 8) * m_tileSize);
		return m_tileTexture(x, y, m_tileSize, m_tileSize);
	}

	/// @brief オートタイルの展開図のテクスチャを返します。
	/// @return オートタイルの展開図のテクスチャ
	[[nodiscard]]
	const Texture& getTileTexture() const noexcept
	{
		return m_tileTexture;
	}

private:

	struct BaseTileIndex
	{
		int8 topLeft, topRight, bottomLeft, bottomRight;
	};

	[[nodiscard]]
	static Image MakeTileImage(const Image& baseTileImage, const BaseTileIndex& index, int32 tileSize)
	{
		const int32 halfTileSize = (tileSize / 2);
		Image image{ Size{ tileSize, tileSize } };
		baseTileImage(0, (tileSize * index.topLeft), halfTileSize, halfTileSize).overwrite(image, 0, 0);
		baseTileImage(halfTileSize, (tileSize * index.topRight), halfTileSize, halfTileSize).overwrite(image, halfTileSize, 0);
		baseTileImage(0, (tileSize * index.bottomLeft + halfTileSize), halfTileSize, halfTileSize).overwrite(image, 0, halfTileSize);
		baseTileImage(halfTileSize, (tileSize * index.bottomRight + halfTileSize), halfTileSize, halfTileSize).overwrite(image, halfTileSize, halfTileSize);
		return image;
	}

	[[nodiscard]]
	static Image MakeTiles(const Image& baseTileImage, int32 tileSize)
	{
		constexpr std::array<BaseTileIndex, 47> BaseTileIndices =
		{ {
			{ 0, 0, 0, 0 }, { 0, 2, 0, 2 },	{ 2, 2, 2, 2 },	{ 2, 0, 2, 0 }, { 0, 0, 1, 1 },	{ 0, 2, 1, 4 },	{ 2, 2, 4, 4 },	{ 2, 0, 4, 1 },
			{ 0, 2, 1, 3 },	{ 2, 0, 3, 1 },	{ 1, 3, 1, 3 },	{ 2, 2, 3, 3 },	{ 1, 1, 1, 1 },	{ 1, 4, 1, 4 },	{ 4, 4, 4, 4 },	{ 4, 1, 4, 1 },
			{ 1, 3, 0, 2 },	{ 3, 1, 2, 0 },	{ 3, 3, 2, 2 },	{ 3, 1, 3, 1 },	{ 1, 1, 0, 0 },	{ 1, 4, 0, 2 },	{ 4, 4, 2, 2 },	{ 4, 1, 2, 0 },
			{ 1, 4, 1, 3 },	{ 4, 1, 3, 1 },	{ 2, 2, 4, 3 },	{ 2, 2, 3, 4 },	{ 4, 4, 4, 3 },	{ 4, 4, 3, 4 },	{ 4, 3, 3, 3 },	{ 3, 4, 3, 3 },
			{ 1, 3, 1, 4 },	{ 3, 1, 4, 1 }, { 4, 3, 2, 2 },	{ 3, 4, 2, 2 },	{ 4, 3, 4, 4 },	{ 3, 4, 4, 4 },	{ 3, 3, 4, 3 },	{ 3, 3, 3, 4 },
			{ 3, 3, 4, 4 },	{ 4, 4, 3, 3 },	{ 4, 3, 4, 3 },	{ 3, 4, 3, 4 },	{ 4, 3, 3, 4 },	{ 3, 4, 4, 3 },	{ 3, 3, 3, 3 }
		} };

		Image image{ Size{ (tileSize * 8), (tileSize * 6) }, Color{ 255, 0 } };

		for (int32 i = 0; i < 47; ++i)
		{
			const int32 x = ((i % 8) * tileSize);
			const int32 y = ((i / 8) * tileSize);
			MakeTileImage(baseTileImage, BaseTileIndices[i], tileSize).overwrite(image, x, y);
		}

		return image;
	}

	[[nodiscard]]
	static uint8 GetTileIndex(uint8 bits) noexcept
	{
		constexpr std::array<uint8, 256> Indices =
		{
			 0,  0,  4,  4,  0,  0,  4,  4,  1,  1,  8,  5,  1,  1,  8,  5,
			 3,  3,  9,  9,  3,  3,  7,  7,  2,  2, 11, 27,  2,  2, 26,  6,
			 0,  0,  4,  4,  0,  0,  4,  4,  1,  1,  8,  5,  1,  1,  8,  5,
			 3,  3,  9,  9,  3,  3,  7,  7,  2,  2, 11, 27,  2,  2, 26,  6,
			20, 20, 12, 12, 20, 20, 12, 12, 16, 16, 10, 32, 16, 16, 10, 32,
			17, 17, 19, 19, 17, 17, 33, 33, 18, 18, 46, 39, 18, 18, 38, 40,
			20, 20, 12, 12, 20, 20, 12, 12, 21, 21, 24, 13, 21, 21, 24, 13,
			17, 17, 19, 19, 17, 17, 33, 33, 35, 35, 31, 43, 35, 35, 45, 37,
			 0,  0,  4,  4,  0,  0,  4,  4,  1,  1,  8,  5,  1,  1,  8,  5,
			 3,  3,  9,  9,  3,  3,  7,  7,  2,  2, 11, 27,  2,  2, 26,  6,
			 0,  0,  4,  4,  0,  0,  4,  4,  1,  1,  8,  5,  1,  1,  8,  5,
			 3,  3,  9,  9,  3,  3,  7,  7,  2,  2, 11, 27,  2,  2, 26,  6,
			20, 20, 12, 12, 20, 20, 12, 12, 16, 16, 10, 32, 16, 16, 10, 32,
			23, 23, 25, 25, 23, 23, 15, 15, 34, 34, 30, 44, 34, 34, 42, 36,
			20, 20, 12, 12, 20, 20, 12, 12, 21, 21, 24, 13, 21, 21, 24, 13,
			23, 23, 25, 25, 23, 23, 15, 15, 22, 22, 41, 29, 22, 22, 28, 14
		};

		return Indices[bits];
	}

	[[nodiscard]]
	static AutoTile CreateFromBaseImage(const Image& baseTileImage_1x5)
	{
		AutoTile autoTiles;
		autoTiles.m_tileSize = baseTileImage_1x5.width();
		autoTiles.m_tileTexture = Texture{ MakeTiles(baseTileImage_1x5, autoTiles.m_tileSize) };
		return autoTiles;
	}

	[[nodiscard]]
	static AutoTile CreateFromTiles(const Image& tiles_8x6)
	{
		AutoTile autoTiles;
		autoTiles.m_tileSize = (tiles_8x6.width() / 8);
		autoTiles.m_tileTexture = Texture{ tiles_8x6 };
		return autoTiles;
	}

	int32 m_tileSize = 1;

	Texture m_tileTexture;
};

/// @brief オートタイルの接続情報
struct AutoTileConnectivity
{
	bool connected[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
};

/// @brief タイル上でカーソルを描きます。
void DrawCursor()
{
	Cursor::RequestStyle(CursorStyle::Hidden);
	const Vec2 cursorPos = Cursor::PosF();
	const Triangle triangle{ cursorPos, (cursorPos + Vec2{ 20, 6 }), (cursorPos + Vec2{ 6, 20 }) };
	triangle.stretched(1.2).movedBy(0.0, 0.5).draw(ColorF{ 0.25 });
	triangle.draw();
}

/// @brief マウスカーソルがあるタイルのインデックスを返します。
/// @param size タイル数
/// @param tileSize タイルのサイズ（ピクセル）
/// @param offset タイルの描画位置のオフセット
/// @return タイルのインデックス。タイル上にカーソルがない場合は none
[[nodiscard]]
Optional<Point> GetCursorIndex(const Size& size, int32 tileSize, const Point& offset)
{
	const Point cursorPos = (Cursor::Pos() - offset);

	if ((not InRange(cursorPos.x, 0, (size.x * tileSize - 1)))
		|| (not InRange(cursorPos.y, 0, (size.y * tileSize - 1))))
	{
		return none;
	}

	return{ cursorPos / tileSize };
}

void Main()
{
	Window::Resize(1280, 720);
	Scene::SetBackground(ColorF{ 0.75 });
	const Texture baseTexture{ U"base.png" };

	const Array<AutoTile> autoTiles =
	{
		AutoTile{ Image{ U"a.png" } },
		AutoTile{ Image{ U"b.png" } },
		AutoTile{ Image{ U"c.png" } },
		AutoTile{ Image{ U"d.png" } },
	};
	size_t autoTileIndex = 0;

	// マップのセルの数
	constexpr Size GridSize{ 20, 20 };

	// マップを描画するときのオフセット
	constexpr Point LayerOffset{ 40, 40 };

	// オートタイルの有無を格納する二次元配列
	Grid<uint32> grid(GridSize, 0);

	// オートタイルの接続情報を格納する二次元配列
	Grid<AutoTileConnectivity> connectivityGrid(GridSize);

	// 選択されているタイルのインデックス
	Optional<Point> selectedTileIndex;

	while (System::Update())
	{
		// 背景の市松模様を描く
		for (int32 y = 0; y < (Scene::Height() / 20); ++y)
		{
			for (int32 x = 0; x < (Scene::Width() / 20); ++x)
			{
				if (IsEven(y + x))
				{
					Rect{ (x * 20), (y * 20), 20 }.draw(ColorF{ 0.7 });
				}
			}
		}

		// 現在のオートタイルの種類
		const auto& autoTile = autoTiles[autoTileIndex];
		const int32 tileSize = autoTile.getTileSize();

		// カーソルでマウスオーバーしているタイルのインデックス
		const auto cursorIndex = GetCursorIndex(grid.size(), tileSize, LayerOffset);

		// クリックでタイルを編集する
		if (cursorIndex && (MouseL.pressed() || MouseR.pressed()))
		{
			if (MouseL.pressed())
			{
				grid[*cursorIndex] = 1;
			}
			else
			{
				grid[*cursorIndex] = 0;
				connectivityGrid[*cursorIndex] = AutoTileConnectivity{};
			}

			selectedTileIndex = *cursorIndex;
		}

		// マップチップを描く
		for (int32 y = 0; y < grid.height(); ++y)
		{
			for (int32 x = 0; x < grid.width(); ++x)
			{
				baseTexture.draw(Point{ x, y } * tileSize + LayerOffset);

				// オートタイルであれば
				if (grid[y][x])
				{
					const auto& connectivity = connectivityGrid[y][x];
					const uint32 c0 = (grid.fetch((y - 1), (x - 1), 1) & static_cast<uint32>(connectivity.connected[0]));
					const uint32 c1 = (grid.fetch((y - 1), x, 1) & static_cast<uint32>(connectivity.connected[1]));
					const uint32 c2 = (grid.fetch((y - 1), (x + 1), 1) & static_cast<uint32>(connectivity.connected[2]));
					const uint32 c3 = (grid.fetch(y, (x - 1), 1) & static_cast<uint32>(connectivity.connected[3]));
					const uint32 c4 = (grid.fetch(y, (x + 1), 1) & static_cast<uint32>(connectivity.connected[4]));
					const uint32 c5 = (grid.fetch((y + 1), (x - 1), 1) & static_cast<uint32>(connectivity.connected[5]));
					const uint32 c6 = (grid.fetch((y + 1), x, 1) & static_cast<uint32>(connectivity.connected[6]));
					const uint32 c7 = (grid.fetch((y + 1), (x + 1), 1) & static_cast<uint32>(connectivity.connected[7]));
					const uint8 bits = static_cast<uint8>((c0 << 7) | (c1 << 6) | (c2 << 5) | (c3 << 4) | (c4 << 3) | (c5 << 2) | (c6 << 1) | c7);
					autoTile.getTile(bits).draw(Point{ x, y } * tileSize + LayerOffset);
				}
				else
				{
					Rect{ (Point{ x, y } * tileSize + LayerOffset), tileSize }.drawFrame(1, ColorF{ 0.5 });
				}
			}
		}

		// カーソルが重なっているタイルを強調表示する
		if (cursorIndex)
		{
			Rect{ (*cursorIndex * tileSize + LayerOffset), tileSize }.draw(ColorF{ 1.0, 0.5, 0.0, 0.5 });
			DrawCursor();
		}

		// オートタイルの展開図を描画する
		Rect{ 720, 40, 256, 192 }.draw();
		autoTile.getTileTexture().draw(720, 40);
		SimpleGUI::RadioButtons(autoTileIndex, { U"A", U"B", U"C", U"D" }, Vec2{ 1020, 40 });

		// タイルが選択されている場合、その接続情報を表示する
		if (selectedTileIndex)
		{
			Rect{ (*selectedTileIndex * tileSize) + LayerOffset, tileSize }.drawFrame(2, 0, Palette::Red);

			// 接続情報を可視化・操作するチェックボックスを描画する
			auto& connectivity = connectivityGrid[*selectedTileIndex];
			for (int32 i = 0; i < 8; ++i)
			{
				int32 t = (i + (3 < i));
				SimpleGUI::CheckBox(connectivity.connected[i], U"", Vec2{ (720 + (t % 3) * 50), (280 + (t / 3) * 40) });
			}
		}
	}
}
