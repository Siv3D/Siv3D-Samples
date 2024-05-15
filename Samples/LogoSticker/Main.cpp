# include <Siv3D.hpp> // Siv3D v0.6.14

BlendState MakeBlendState()
{
	BlendState blendState = BlendState::Default2D;
	blendState.srcAlpha = Blend::SrcAlpha;
	blendState.dstAlpha = Blend::DestAlpha;
	blendState.opAlpha = BlendOp::Max;
	return blendState;
}

// Siv3D ロゴの多角形を作成する関数
MultiPolygon CreateLogoPolygon()
{
	const Array<Bezier3> beziers =
	{
		Bezier3{ Vec2{ -230, 129 }, Vec2{ 4, 190 }, Vec2{ 173, 75 }, Vec2{ 173, -23 } },
		Bezier3{ Vec2{ 173, -23 }, Vec2{ 173, -106 }, Vec2{ 67, -159 }, Vec2{ 25, -56 } },
		Bezier3{ Vec2{ 1, -55 }, Vec2{ 56, -176 }, Vec2{ 226, -135 }, Vec2{ 226, 4 } },
		Bezier3{ Vec2{ 226, 4 }, Vec2{ 226, 140 }, Vec2{ 20, 230 }, Vec2{ -230, 129 } }
	};

	Array<Vec2> points;

	for (auto& bezier : beziers)
	{
		LineString lineString = bezier.getLineString();
		lineString.pop_back();
		points.append(lineString);
	}

	MultiPolygon multiPolygon;
	multiPolygon << Geometry2D::Or(Polygon{ points }, Circle{ 49, -42, 50 }.asPolygon())[0];
	multiPolygon << multiPolygon[0].rotated(180_deg);
	return multiPolygon;
}

void UnpremultiplyAlpha(Image& image)
{
	for (auto& pixel : image)
	{
		if (pixel.a)
		{
			ColorF color = pixel;
			color.r /= color.a;
			color.g /= color.a;
			color.b /= color.a;
			pixel = color;
		}
	}
}

Image ReadImage(const MSRenderTexture& renderTexture)
{
	Image image;
	renderTexture.readAsImage(image);
	UnpremultiplyAlpha(image);
	return image;
}

void DrawBackground()
{
	constexpr int32 CellSize = 80;
	const double t = Math::Fraction(Scene::Time() * 0.2);

	for (int32 y = -1; y < (Scene::Height() / CellSize); ++y)
	{
		for (int32 x = 0; x <= (Scene::Width() / CellSize); ++x)
		{
			if (IsEven(y + x))
			{
				RectF{ (x * CellSize), (y * CellSize), CellSize }
					.movedBy((-t * CellSize), (t * CellSize))
					.draw(ColorF{ 0.7, 0.72, 0.74 });
			}
		}
	}
}

struct CharacterInfo
{
	char32 ch;
	Vec2 pos;
	double angle;
	double scale;
	Color color;
};

struct Item
{
	MultiPolygon polygons;
	Color color;
};

void Main()
{
	Window::Resize(1280, 720);
	Scene::SetBackground(ColorF{ 0.86, 0.88, 0.9 });

	constexpr Color AlphabetColor1 = Palette::Lightskyblue;
	constexpr Color AlphabetColor2 = Palette::Gold;
	constexpr Color HiraganaColor1{ 71, 185, 255 };
	constexpr Color HiraganaColor2{ 255, 200, 0 };

	// 文字
	const Font font{ 240, Typeface::Black };

	// text それぞれの文字に位置、角度、拡大率、色を割り当てる
	const Array<CharacterInfo> characters =
	{
		{ U'S', Vec2{400, 60}, 10_deg, 1, AlphabetColor1},
		{ U'i', Vec2{ 560, 120 }, 5_deg, 0.8, AlphabetColor1 },
		{ U'v', Vec2{ 620, 160 }, -15_deg, 1, AlphabetColor1 },
		{ U'3', Vec2{ 550, 300 }, 10_deg, 1, AlphabetColor2 },
		{ U'D', Vec2{ 700, 310 }, -10_deg, 1, AlphabetColor2 },
		{ U'し', Vec2{ 380, 270 }, 5_deg, 0.3, HiraganaColor1 },
		{ U'ぶ', Vec2{ 440, 280 }, 0, 0.3, HiraganaColor1 },
		{ U'す', Vec2{ 470, 500 }, 10_deg, 0.3, HiraganaColor2 },
		{ U'り', Vec2{ 550, 510 }, 5_deg, 0.3, HiraganaColor2 },
		{ U'ー', Vec2{ 620, 530 }, 0, 0.3, HiraganaColor2 },
		{ U'で', Vec2{ 700, 510 }, -5_deg, 0.3, HiraganaColor2 },
		{ U'ぃ', Vec2{ 780, 520 }, -10_deg, 0.25, HiraganaColor2 },
		{ U'ー', Vec2{ 840, 500 }, -20_deg, 0.3, HiraganaColor2 },
	};

	//　ステッカーの部品を格納する配列
	Array<Item> items;
	{
		for (const auto& character : characters)
		{
			Item item
			{
				.polygons = font.renderPolygon(character.ch).polygons,
				.color = character.color
			};

			for (auto& p : item.polygons)
			{
				p = p.calculateRoundBuffer(4)
					.scale(character.scale)
					.rotate(character.angle)
					.moveBy(character.pos);
			}

			items << std::move(item);
		}

		// ロゴマーク
		items.emplace_back(CreateLogoPolygon().scale(0.3).rotate(25_deg).moveBy(436, 420), Color{ 36, 168, 249 });

		// 円
		items.emplace_back(MultiPolygon{ Circle(837,220,50).asPolygon(36) }, Palette::Orchid);

		// 尾
		items.emplace_back(MultiPolygon{ Polygon{{893,360},{924,309},{916,215},{990,300},{1011,400},{957,482}, {899,445} } }, Palette::Lightgreen);

		// バツ印
		items.emplace_back(MultiPolygon{ Shape2D::Plus(30,18,{877,305},15_deg) }, Palette::Gainsboro);
		items.emplace_back(MultiPolygon{ Shape2D::Plus(30,18,{344,352},-15_deg) }, Palette::Gainsboro);
	}

	// ステッカー部品の形状を太らせて結合
	MultiPolygon backgroundPolygons;

	for (const auto& item : items)
	{
		for (const auto& polygon : item.polygons)
		{
			backgroundPolygons = Geometry2D::Or(backgroundPolygons, polygon.calculateRoundBuffer(20));
		}
	}

	// 穴を消す
	for (auto& polygon : backgroundPolygons)
	{
		polygon = Polygon{ polygon.outer() };
	}

	// MSRenderTextureに描画
	const MSRenderTexture renderTexture{ Size{ 1280, 720 }, ColorF{ 0.0, 0.0 } };
	{
		const ScopedRenderTarget2D target{ renderTexture };
		const ScopedRenderStates2D blend{ MakeBlendState() };

		// 影の描画
		{
			const Transformer2D t{ Mat3x2::Translate(10, 20) };
			backgroundPolygons.draw(Palette::Steelblue);
		}

		// 白地の描画
		backgroundPolygons.draw();

		// 部品の描画
		for (auto& item : items)
		{
			item.polygons.draw(item.color);
		}

		// 円の模様の描画
		{
			Circle{ 837, 220, 40 }.draw(Palette::Plum);
			Shape2D::Star(23, { 837,220 }, 15_deg).asPolygon()
				.calculateRoundBuffer(6).draw(Color{ 255, 255, 135 });
		}

		// 尾の模様の描画
		{
			const Triangle tailPattern{ {0,-30},{10,0},{-10,0} };
			tailPattern.rotatedAt({}, 110_deg).movedBy(924, 309).draw(Palette::Whitesmoke);
			tailPattern.scaledAt({}, 0.7).rotatedAt({}, 80_deg).movedBy(922, 259).draw(Palette::Whitesmoke);
			tailPattern.scaledAt({}, 0.8).rotatedAt({}, 130_deg).movedBy(902, 347).draw(Palette::Whitesmoke);
			Polygon{ {893,360},{924,309},{916,215},{934,314} }.draw(Palette::Dimgray);
			Polygon{ {893,360},{957,482}, {899,445} }.draw(Palette::Darkseagreen.withAlpha(128));
		}
	}

	// MSRenderTexture への描画を完了する
	{
		Graphics2D::Flush();
		renderTexture.resolve();
	}

	// MSRenderTexture の内容から Image を作成する
	const Image image = ReadImage(renderTexture);

	const Texture texture{ image };

	while (System::Update())
	{
		// 背景の格子を描画する
		DrawBackground();

		// 完成したロゴを描画する
		texture.draw();

		// ロゴ画像の画像保存ボタン
		if (SimpleGUI::Button(U"画像を保存", Vec2{ 40, 40 }))
		{
			image.saveWithDialog();
		}
	}
}
