# include <Siv3D.hpp> // Siv3D v0.6.13
# include "Numpad.hpp"

void Main()
{
	Window::Resize(1280, 720);
	Scene::SetBackground(ColorF{ 0.6, 0.8, 0.7 });
	const Font font = SimpleGUI::GetFont();
	const Font iconFont{ 48, Typeface::Icon_MaterialDesign };
	const Font font2{ FontMethod::MSDF, 40, U"example/font/RocknRoll/RocknRollOne-Regular.ttf" };
	font2.addFallback(iconFont);

	constexpr RoundRect DigitsRectA{ 50, 110, 310, 60, 4 };
	constexpr RoundRect DigitsRectB{ 400, 110, 304, 60, 30 };
	constexpr RoundRect DigitsRectC{ 750, 110, 304, 60, 30 };

	// 数値パッド A
	Numpad numpadA;

	// 数値パッド B
	Numpad numpadB{ { .keySize = SizeF{ 70, 48 }, .keyMargin = { 8, 8 }, .roundRadius = 8.0,
		.keyColor = ColorF{ 0.9, 0.95, 1.0 }, .keyHoveredColor = ColorF{ 0.8, 0.9, 1.0 }, .textFont = font2, .fontScale = 1.25, .maxDigits = 9 } };

	// 数値パッド C
	Numpad numpadC{ { .keySize = SizeF{ 70, 48 }, .keyMargin = { 8, 8 }, .roundRadius = 8.0,
		.keyColor = ColorF{ 0.9, 0.95, 1.0 }, .keyHoveredColor = ColorF{ 0.8, 0.9, 1.0 }, .textFont = font2, .fontScale = 1.25, .maxDigits = 9 } };

	double aValue = 0.0, bValue = 0.0, cValue = 123; // cValue は -999 以上 999 以下の整数

	while (System::Update())
	{
		ClearPrint();
		Print << U"a: {}"_fmt(aValue = numpadA.getFloat());
		Print << U"b: {}"_fmt(bValue = numpadB.getFloat());
		Print << U"c: {}"_fmt(cValue);

		// 更新
		{
			// A
			if ((not numpadA.isOpen()) && DigitsRectA.leftClicked())
			{
				numpadB.close();
				numpadC.close();
				numpadA.open(Vec2{ 40, 180 });
			}
			else if (numpadA.update(Numpad::AllowKeyInput) || (numpadA.isOpen() && DigitsRectA.leftClicked()))
			{
				aValue = numpadA.getFloat();
				numpadA.close();
			}

			// B
			if ((not numpadB.isOpen()) && DigitsRectB.leftClicked())
			{
				numpadA.close();
				numpadC.close();
				numpadB.open(Vec2{ 392, 180 });
			}
			else if (numpadB.update(Numpad::AllowKeyInput) || (numpadB.isOpen() && DigitsRectB.leftClicked()))
			{
				bValue = numpadB.getFloat();
				numpadB.close();
			}

			// C
			if ((not numpadC.isOpen()) && DigitsRectC.leftClicked())
			{
				numpadA.close();
				numpadB.close();
				numpadC.open(Vec2{ 742, 180 }, cValue);
			}
			else if (numpadC.update(Numpad::AllowKeyInput) || (numpadC.isOpen() && DigitsRectC.leftClicked()))
			{
				cValue = Clamp(static_cast<int32>(numpadC.getInt()), -999, 999);
				numpadC.close();
			}
		}

		// 描画
		{
			// A
			{
				if (DigitsRectA.draw().mouseOver())
				{
					Cursor::RequestStyle(CursorStyle::Hand);
				}

				const String digits = (numpadA.isOpen() ? numpadA.getText() : numpadA.formatValue(aValue));
				font(digits).draw(32, Arg::rightCenter(DigitsRectA.rightCenter().movedBy(-8, -2)), ColorF{ 0.11 });
				numpadA.draw();
			}

			// B
			{
				if (DigitsRectB.draw(ColorF{ 0.2, 0.0, 0.5 }).mouseOver())
				{
					Cursor::RequestStyle(CursorStyle::Hand);
				}

				const int32 intDigits = static_cast<int32>(ToString(static_cast<int64>(bValue)).length());
				const String digits = (numpadB.isOpen() ? numpadB.withThousandsSeparators() : ThousandsSeparate(bValue, (numpadA.getMaxDigits() - intDigits)));
				font2(digits).draw(32, Arg::rightCenter(DigitsRectB.rightCenter().movedBy(-24, -2)), ColorF{ 0.98 });
				numpadB.draw();
			}

			// C
			{
				if (DigitsRectC.draw(ColorF{ 0.2, 0.0, 0.5 }).mouseOver())
				{
					Cursor::RequestStyle(CursorStyle::Hand);
				}

				const String digits = (numpadC.isOpen() ? numpadC.getText() : numpadA.formatValue(cValue));
				font2(digits).draw(32, Arg::rightCenter(DigitsRectC.rightCenter().movedBy(-24, -2)), ColorF{ 0.98 });
				numpadC.draw();
			}
		}
	}
}
