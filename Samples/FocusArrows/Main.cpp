# include <Siv3D.hpp> // OpenSiv3D v0.6.5

void Main()
{
	Window::Resize(1280, 720);

	Transition transition{ 0.3s, 0.3s };

	while (System::Update())
	{
		transition.update(MouseL.pressed());

		const double exp = (1.0 + transition.easeOut(Easing::Quad));

		const ScopedRenderStates2D bled{ BlendState::Additive };

		for (int32 y = 0; y < (720 / 120 + 1); ++y)
		{
			for (int32 x = 0; x < (1280 / 120 + 1); ++x)
			{
				Vec2 pos{ (40 + x * 120), (y * 120) };

				const double distance = (Cursor::PosF() - pos).length();

				const Vec2 dir = (distance ? ((Cursor::PosF() - pos) / distance) : Vec2::Up());

				pos += (dir * Pow(distance, exp) * 0.0005);

				const Vec2 to = (pos + dir * 40);

				const Vec2 from = (pos - dir * 40);

				const ColorF color = HSV{ (x * 155 + y * 135) };

				Shape2D::Arrow(from, to, 15, Vec2{ 30.0, 50.0 }).draw(ColorF{ color, 0.4 }).drawFrame(3, color);
			}
		}
	}
}