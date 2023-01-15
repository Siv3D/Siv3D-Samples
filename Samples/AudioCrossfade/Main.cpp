#include <Siv3D.hpp> // OpenSiv3D v0.6.6

void Main()
{
	// オーディオグループを作成
	const AudioGroup group{ Audio{ Audio::Stream, U"bgm/A.ogg", Loop::Yes }, Audio{ Audio::Stream, U"bgm/B.ogg", Loop::Yes } };

	// B の音量を 0 に設定
	group.setVolumeOne(1, 0.0);

	// オーディオグループを再生
	group.playAll();

	// メインで聞かせている BGM
	int32 bgmIndex = 0;

	// フェード時間
	constexpr Duration FadeTime = 2.5s;

	// 移動する円の位置計算用
	Transition transition{ FadeTime, FadeTime };

	// FFT 結果の格納先
	FFTResult busFFT;

	while (System::Update())
	{
		// メインで聞かせている BGM に応じて円を移動させる
		transition.update(bgmIndex == 1);

		// FFT を計算し, 結果を格納する
		GlobalAudio::BusGetFFT(MixBus0, busFFT);

		// A を聞かせる
		if (SimpleGUI::Button(U"A", Vec2{ 100, 100 }, 80, (bgmIndex != 0)))
		{
			group.fadeVolumeOne(0, 1.0, FadeTime);
			group.fadeVolumeOne(1, 0.00, FadeTime);
			bgmIndex = 0;
		}

		// B を聞かせる
		if (SimpleGUI::Button(U"B", Vec2{ 520, 100 }, 80, (bgmIndex != 1)))
		{
			group.fadeVolumeOne(1, 1.0, FadeTime);
			group.fadeVolumeOne(0, 0.00, FadeTime);
			bgmIndex = 1;
		}

		// 再生 BGM を可視化する
		{
			Line{ 200, 118, 500, 118 }.draw(2).draw();
			Circle{ Math::Lerp(200, 500, transition.value()), 118, 12 }.draw();
		}

		// FFT 結果を可視化する
		for (size_t i = 0; i < Min<size_t>(busFFT.buffer.size(), 125); ++i)
		{
			const double s = busFFT.buffer[i];
			RectF{ Arg::bottomLeft(100 + 4 * i, 300), 4, Math::Pow(s, 0.5) * 20 }.draw(HSV{ 240 - i * 2 });
		}
	}

	// すべてのオーディオをフェードアウトして終了
	GlobalAudio::FadeVolume(0.0, 0.2s);
	System::Sleep(0.3s);
}
