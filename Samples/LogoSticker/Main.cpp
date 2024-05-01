# include <Siv3D.hpp> // Siv3D v0.6.14

// チュートリアル 41.3
BlendState MakeBlendState()
{
	BlendState blendState = BlendState::Default2D;
	blendState.srcAlpha = Blend::SrcAlpha;
	blendState.dstAlpha = Blend::DestAlpha;
	blendState.opAlpha = BlendOp::Max;
	return blendState;
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

// ロゴの多角形を作成
MultiPolygon createLogoPolygon() {
	const Array<Bezier3> beziers = {
		Bezier3(Vec2(-230, 129), Vec2(4, 190), Vec2(173, 75), Vec2(173, -23)),
		Bezier3(Vec2(173, -23), Vec2(173, -106), Vec2(67, -159), Vec2(25, -56)),
		Bezier3(Vec2(1, -55), Vec2(56, -176), Vec2(226, -135), Vec2(226, 4)),
		Bezier3(Vec2(226, 4), Vec2(226, 140), Vec2(20, 230), Vec2(-230, 129))
	};

	Array<Vec2> points;
	for (auto& bezier : beziers) {
		LineString lineString = bezier.getLineString();
		lineString.pop_back();
		points.append(lineString);
	}

	MultiPolygon multiPolygon(2);
	multiPolygon[0] = Geometry2D::Or(Polygon(points), Circle(49, -42, 50).asPolygon())[0];
	multiPolygon[1] = multiPolygon[0].rotated(180_deg);

	return multiPolygon;
}

void Main()
{
	Window::Resize(1280, 720);

	const String text = U"Siv3Dしぶすりーでぃー";
	struct PosAngleScaleColor
	{
		Vec2 pos;
		double angle;
		double scale;
		Color color;
	};

	constexpr Color sivPhoneticColor = Color{ 71, 185, 255 };
	constexpr Color threeDPhoneticColor = Color{ 255, 200, 0 };

	//textそれぞれの文字に位置、角度、拡大率、色を割り当てる
	const Array<PosAngleScaleColor> posAngleScaleColors{
		{ Vec2(400, 60),10_deg, 1, Palette::Lightskyblue },//s
		{ Vec2(560, 120), 5_deg, 0.8, Palette::Lightskyblue },//i
		{ Vec2(620, 160), -15_deg, 1, Palette::Lightskyblue },//v
		{ Vec2(550, 300), 10_deg, 1, Palette::Gold },//3
		{ Vec2(700, 310), -10_deg, 1, Palette::Gold },//D

		{ Vec2(380, 270), 5_deg, 0.3, sivPhoneticColor },//し
		{ Vec2(440, 280), 0, 0.3, sivPhoneticColor },//ぶ
		{ Vec2(470, 500), 10_deg, 0.3, threeDPhoneticColor },//す
		{ Vec2(550, 510), 5_deg, 0.3, threeDPhoneticColor },//り
		{ Vec2(620, 530), 0, 0.3, threeDPhoneticColor },//ー
		{ Vec2(700, 510), -5_deg, 0.3, threeDPhoneticColor },//で
		{ Vec2(780, 520), -10_deg, 0.25, threeDPhoneticColor },//ぃ
		{ Vec2(840, 500), -20_deg, 0.3, threeDPhoneticColor },//ー
	};

	struct StickerPart
	{
		MultiPolygon polygons;
		Color color;
	};

	//ステッカーの部品を格納する配列
	Array<StickerPart> parts;

	
	//文字
	const Font font(240, Typeface::Black);
	for (auto [i,chara] : Indexed(text)) {
		StickerPart part = {
			.polygons = font.renderPolygon(chara).polygons,
			.color = posAngleScaleColors[i].color
		};
		for (auto& p : part.polygons) {
			p = p.calculateRoundBuffer(4).scale(posAngleScaleColors[i].scale).rotate(posAngleScaleColors[i].angle).moveBy(posAngleScaleColors[i].pos);
		}
		parts.emplace_back(std::move(part));
	}

	//ロゴマーク
	parts.emplace_back(createLogoPolygon().scale(0.3).rotate(25_deg).moveBy(436, 420), Color{ 36, 168, 249 });

	//円
	parts.emplace_back(MultiPolygon{ Circle(837,220,50).asPolygon(36) }, Palette::Orchid);

	//尾
	parts.emplace_back(MultiPolygon{ Polygon{{893,360},{924,309},{916,215},{990,300},{1011,400},{957,482}, {899,445} } }, Palette::Lightgreen);

	//バツ印
	parts.emplace_back(MultiPolygon{ Shape2D::Plus(30,18,{877,305},15_deg) }, Palette::Gainsboro);
	parts.emplace_back(MultiPolygon{ Shape2D::Plus(30,18,{344,352},-15_deg) }, Palette::Gainsboro);


	//ステッカー部品の形状を太らせて結合
	MultiPolygon combinedBufferedPolygons;
	for (auto& part : parts) {
		for (auto& p : part.polygons) {
			combinedBufferedPolygons = Geometry2D::Or(combinedBufferedPolygons, p.calculateRoundBuffer(20));
		}
	}

	//穴を消す
	for (auto& p : combinedBufferedPolygons) {
		p = Polygon(p.outer());
	}

	//MSRenderTextureに描画
	MSRenderTexture renderTexture(1280, 720, Color(0,0));
	{
		ScopedRenderTarget2D target(renderTexture);
		ScopedRenderStates2D blend{ MakeBlendState() };

		{
			//影
			Transformer2D t(Mat3x2::Translate(10, 20));
			combinedBufferedPolygons.draw(Palette::Steelblue);
		}
		//白地
		combinedBufferedPolygons.draw(Palette::White);

		//部品の描画
		for (auto& part : parts) {
			part.polygons.draw(part.color);
		}

		//円の模様
		Circle(837, 220, 40).draw(Palette::Plum);
		Shape2D::Star(23, { 837,220 }, 15_deg).asPolygon().calculateRoundBuffer(6).draw(Color{ 255, 255, 135 });

		//尾の模様
		Triangle tailPattern{ {0,-30},{10,0},{-10,0} };
		tailPattern.rotatedAt({}, 110_deg).movedBy(924, 309).draw(Palette::Whitesmoke);
		tailPattern.scaledAt({}, 0.7).rotatedAt({}, 80_deg).movedBy(922, 259).draw(Palette::Whitesmoke);
		tailPattern.scaledAt({}, 0.8).rotatedAt({}, 130_deg).movedBy(902, 347).draw(Palette::Whitesmoke);
		Polygon{ {893,360},{924,309},{916,215},{934,314} }.draw(Palette::Dimgray);
		Polygon{ {893,360},{957,482}, {899,445} }.draw(Palette::Darkseagreen.withAlpha(128));
	}

	Graphics2D::Flush();
	renderTexture.resolve();

	//renderTextureに描画したものをImageにする
	Image image;
	renderTexture.readAsImage(image);
	UnpremultiplyAlpha(image);

	const Texture texture{ image };
	Scene::SetBackground(ColorF{ 0.86, 0.88, 0.9 });
	
	while (System::Update())
	{
		//背景の格子を描画
		for (int32 y = 0; y < (Scene::Height() / 80); ++y)
		{
			for (int32 x = 0; x < (Scene::Width() / 80); ++x)
			{
				if (IsEven(y + x))
				{
					Rect{ (x * 80), (y * 80), 80 }.draw(ColorF{ 0.7, 0.72, 0.74 });
				}
			}
		}

		//画像保存ボタン
		if (SimpleGUI::Button(U"画像を保存", Vec2{ 20, 20 }))
		{
			auto path = Dialog::SaveImage();
			if (path)
			{
				image.save(*path);
			}
		}

		texture.draw();
	}
}
