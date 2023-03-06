# include <Siv3D.hpp> // OpenSiv3D v0.6.6

namespace s3d::ChatGPT
{
	[[nodiscard]]
	inline String Chat(const String input, const String SECRET_API_KEY)
	{
		if (not SECRET_API_KEY)
		{
			Print << U"API key is empty.";
			return{};
		}

		JSON chat;
		chat[U"model"] = U"gpt-3.5-turbo";
		chat[U"messages"].push_back({ { U"role", U"user" }, { U"content", input } });
		const std::string data = chat.formatUTF8();

		const HashTable<String, String> headers =
		{
			{ U"Content-Type", U"application/json" },
			{ U"Authorization", (U"Bearer " + SECRET_API_KEY) },
		};

		const FilePath SavePath = U"result.json";

		if (const auto response = SimpleHTTP::Post(U"https://api.openai.com/v1/chat/completions", headers, data.data(), data.size(), SavePath))
		{
			if (not response.isOK())
			{
				Print << U"status code: {}"_fmt(FromEnum(response.getStatusCode()));
				return{};
			}

			const JSON result = JSON::Load(SavePath);
			return result[U"choices"][0][U"message"][U"content"].getString();
		}
		else
		{
			Print << U"FAILED";
			return{};
		}
	}

	[[nodiscard]]
	inline AsyncTask<String> ChatAsync(const String input, const String SECRET_API_KEY)
	{
		return Async(Chat, input, SECRET_API_KEY);
	}
}

/// @brief 物語
struct Story
{
	/// @brief タイトル
	String title;

	/// @brief 内容
	String story;

	/// @brief 絵文字
	Array<Texture> emojis;

	/// @brief 肯定的なレビュー
	String positiveReview;

	/// @brief 批判的なレビュー
	String negativeReview;
};

/// @brief 絵文字列を絵文字テクスチャの配列に変換します。
/// @param emojis 絵文字列
/// @return 絵文字テクスチャの配列
Array<Texture> ParseEmojis(String emojis)
{
	const Font font{ 10, Typeface::ColorEmoji };
	Array<Texture> results;

	while (emojis)
	{
		for (int32 length = 10; 0 <= length; --length)
		{
			if (length == 0)
			{
				emojis.pop_front();
				break;
			}

			if (const String chars = emojis.substr(0, length);
				font.hasGlyph(chars))
			{
				if (Texture texture{ Emoji{ chars } })
				{
					results << texture;
				}

				emojis.pop_front_N(chars.size());
				break;
			}
		}
	}

	return results;
}

void Main()
{
	Window::Resize(1280, 720);
	Scene::SetBackground(ColorF{ 0.6, 0.8, 0.7 });

	// テキスト用のフォント
	const Font font{ FontMethod::MSDF, 40, Typeface::Medium };

	// テキストの色
	constexpr ColorF TextColor{ 0.08 };

	constexpr Rect TitleRect{ 280, 20, 920, 60 };
	constexpr Rect StoryRect{ 240, 90, 1000, 380 };
	constexpr Rect Review1Rect{ 250, 480, 460, 200 };
	constexpr Rect Review2Rect{ 780, 480, 460, 200 };

	const Texture Clapper{ U"🎬"_emoji };
	const Texture Hourglass{ U"⌛"_emoji };
	const Texture Reviwer1{ U"😊"_emoji };
	const Texture Reviwer2{ U"🤔"_emoji };

	// API キーは秘密にする。
	// 誤って API キーをコミットしないよう、環境変数に API キーを設定すると良い（適用には PC の再起動が必要）
	const String SECRET_API_KEY = EnvironmentVariable::Get(U"MY_OPENAI_API_KEY");
	//const String SECRET_API_KEY = U"sk-?????????????????????????????????????????????????";

	// 物語のキーワードを入力するテキストボックス
	std::array<TextEditState, 4> keywords;

	// テキストボックスの移動用の変数
	Optional<size_t> avtivateNextTextBox;

	// 物語の結末の選択肢
	const Array<String> options = { U"幸せな結末", U"悲しい結末", U"意外な結末", U"不思議な結末" };
	size_t index = 0;

	// レビュー文のアニメーション用ストップウォッチ
	Stopwatch stopwatch;

	// 非同期タスク
	AsyncTask<String> task;

	// 物語データ
	Optional<Story> story;

	while (System::Update())
	{
		// 左上のアイコンを描画する
		Clapper.scaled(0.75).drawAt(120, 70);

		// タブキーが押されたら次のテキストボックスへ移動する
		if (avtivateNextTextBox)
		{
			keywords[*avtivateNextTextBox].active = true;
			avtivateNextTextBox.reset();
		}

		// テキストボックスの処理
		for (int32 i = 0; i < 4; ++i)
		{
			const bool previous = keywords[i].active;
			SimpleGUI::TextBox(keywords[i], Vec2{ 30, (140 + i * 40) }, 180);

			// 非アクティブ化された
			if (previous && (keywords[i].active == false) && (keywords[i].tabKey) && (i < 3))
			{
				avtivateNextTextBox = (i + 1);
			}
		}

		// 結末を選択するラジオボタン
		SimpleGUI::RadioButtons(index, options, Vec2{ 30, 320 }, 180);

		if (SimpleGUI::Button(U"物語を作成", Vec2{ 30, 500 }, 180,
			(keywords[0].text && keywords[1].text && keywords[2].text && keywords[3].text && (not task.isValid())))) // 4 つのキーワードが入力されている
		{
			ClearPrint();
			story.reset();

			const String text = UR"(「{}」「{}」「{}」「{}」をテーマにした{}の映画の物語を1つ作ってください。また、短いタイトルと、物語に沿った絵文字、肯定的なレビュー、批判的なレビューを書いてください。ただし、次のような JSON 形式で日本語で出力してください。回答には JSON データ以外を含めないでください。
{{　"title": "", "story1" : "", "story2" : "", "story3" : "", "emojis" : "", "review_positive" : "", "review_negative" : "" }})"_fmt(
				keywords[0].text, keywords[1].text, keywords[2].text, keywords[3].text, options[index]);
			task = ChatGPT::ChatAsync(text, SECRET_API_KEY);
		}

		// ChatGPT の応答を待つ間はローディング画面を表示する
		if (task.isValid())
		{
			Hourglass.rotated(Scene::Time() * 120_deg).drawAt(StoryRect.centerX(), Scene::Center().y);
		}

		if (task.isReady())
		{
			if (const String output = task.get())
			{
				// ChatGPT の返答メッセージに含まれる JSON をパースする
				if (const JSON json = JSON::Parse(output))
				{
					// 指定したフォーマットになっているかを確認する
					if ((json.hasElement(U"title") && json[U"title"].isString())
						&& (json.hasElement(U"story1") && json[U"story1"].isString())
						&& (json.hasElement(U"story2") && json[U"story2"].isString())
						&& (json.hasElement(U"story3") && json[U"story3"].isString())
						&& (json.hasElement(U"emojis") && json[U"emojis"].isString())
						&& (json.hasElement(U"review_positive") && json[U"review_positive"].isString())
						&& (json.hasElement(U"review_negative") && json[U"review_negative"].isString()))
					{
						story = Story
						{
							.title = json[U"title"].getString(),
							.story = (json[U"story1"].getString() + '\n' + json[U"story2"].getString() + '\n' + json[U"story3"].getString()),
							.emojis = ParseEmojis(json[U"emojis"].getString()),
							.positiveReview = json[U"review_positive"].getString(),
							.negativeReview = json[U"review_negative"].getString(),
						};
					}

					// レビュー文のアニメーション用ストップウォッチを始動
					stopwatch.restart();
				}
			}
		}

		if (story)
		{
			// 物語を表示する
			{
				TitleRect.rounded(30).draw();
				StoryRect.rounded(10).draw();
				Review1Rect.rounded(10).draw();
				Review2Rect.rounded(10).draw();
				font(story->title).drawAt(36, TitleRect.center(), TextColor);
				font(story->story).draw(22, StoryRect.stretched(-16), TextColor);

				// ストーリーの挿絵を描画する
				if (story->emojis)
				{
					const int32 size = (StoryRect.w / static_cast<int32>(story->emojis.size()));
					Vec2 pos = StoryRect.bl().movedBy((size / 2), (-size / 2));
					for (const auto& emoji : story->emojis)
					{
						emoji.resized(size * 0.8).drawAt(pos, ColorF{ 1.0 , 0.2 });
						pos.x += size;
					}
				}
			}

			// レビュー文をアニメーションさせて表示する
			{
				const int32 charCount = (stopwatch.ms() / 80);
				font(story->positiveReview.substr(0, charCount)).draw(20, Review1Rect.stretched(-12, -20, -12, -48), TextColor);
				font(story->negativeReview.substr(0, charCount)).draw(20, Review2Rect.stretched(-12, -20, -12, -48), TextColor);
				Reviwer1.scaled(0.8).drawAt(Review1Rect.bl().movedBy(-10, -36));
				Reviwer2.scaled(0.8).drawAt(Review2Rect.bl().movedBy(-10, -36));
			}
		}
	}

	if (task.isValid())
	{
		task.wait();
	}
}
