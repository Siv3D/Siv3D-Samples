# include <Siv3D.hpp> // OpenSiv3D v0.6.10

// Google Apps ScriptのURL
constexpr static StringView ScoreboardAPIUrl = U"https://script.google.com/macros/s/AKfycbw3cQZuo5XKxCgm5vH9ig3rd4A1BG5SVm6eai-3hsT-hdEEYx5lRz20lcBv_dbxuUq-eg/exec";

/// @brief Jsonデータをスコアボードとして読み込む
/// @param json Jsonデータ
/// @param scoreboard 更新するスコアボード
void ParseScoreboardJson(const JSON& json, Array<std::pair<String, double>>& scoreboard)
{
	scoreboard.clear();

	if (not json.isArray())
	{
		return;
	}

	for (const auto& [key, value] : json)
	{
		if (not value.isObject() ||
			not value.hasElement(U"username") ||
			not value.hasElement(U"score"))
		{
			continue;
		}

		const auto& usernameJson = value[U"username"];
		const auto& scoreJson = value[U"score"];

		if (not usernameJson.isString() ||
			not scoreJson.isNumber())
		{
			continue;
		}

		scoreboard.emplace_back(std::pair<String, double>{
			usernameJson.get<String>(),
			scoreJson.get<double>()
		});
	}
}

/// @brief スコアボードを整形したSimpleTableを作成
/// @param scoreboard スコアボード
/// @return 整形したSimpleTable
SimpleTable CreateScoreboardTable(const Array<std::pair<String, double>>& scoreboard)
{
	SimpleTable table{ 3, SimpleTable::Style{ .variableWidth = true } };

	table.push_back_row(Array<String>{
		U"順位", U"ユーザー名", U"スコア"
	});

	int32 rank = 1;
	for (auto& [username, score] : scoreboard)
	{
		table.push_back_row(Array<String>{
			Format(rank), username, Format(score)
		});
		rank++;
	}

	return table;
}

/// @brief スコアボードのJsonを取得するタスクを作成
/// @param count 取得上限数
/// @return タスク
AsyncHTTPTask GetScoreboardJsonAsync(int32 count = 10)
{
	String requestUrl = U"{}?count={}"_fmt(
		ScoreboardAPIUrl,
		count
	);
	Console << U"Get: {}"_fmt(requestUrl);
	return SimpleHTTP::GetAsync(requestUrl, { });
}

/// @brief 新しいスコアを送信するタスクを作成
/// @param username ユーザー名
/// @param score スコア
/// @return タスク
AsyncHTTPTask PushScoreAsync(const StringView username, double score)
{
	String requestUrl = U"{}?username={}&score={}"_fmt(
		ScoreboardAPIUrl,
		PercentEncode(username),
		PercentEncode(Format(score))
	);
	HashTable<String, String> headers = {
		{U"Content-Type", U"application/x-www-form-urlencoded; charset=UTF-8"}
	};
	Console << U"Push: {}"_fmt(requestUrl);
	return SimpleHTTP::PostAsync(requestUrl, headers, nullptr, 0);
}

void Main()
{
	Scene::SetBackground(Palette::Dimgray);
	
	SimpleTable scoreboardTable;

	Optional<AsyncHTTPTask> scoreboardGetTask = GetScoreboardJsonAsync();
	Optional<AsyncHTTPTask> scoreboardPostTask;

	TextEditState usernameInput(U"ユーザー{}"_fmt(Random(100)));

	while (System::Update())
	{
		//// AsyncHTTPTaskの処理 ////

		// 送信処理
		if (scoreboardPostTask &&
			scoreboardPostTask->isReady())
		{
			auto& response = scoreboardPostTask->getResponse();
			Console << U"Response: {}"_fmt(response.getStatusLine());

			if (response.isOK())
			{
				// スコアを送信したあと、スコアボードを更新する
				scoreboardGetTask = GetScoreboardJsonAsync();
			}

			scoreboardPostTask.reset();
		}

		// 取得処理
		if (scoreboardGetTask &&
			scoreboardGetTask->isReady())
		{
			auto& response = scoreboardGetTask->getResponse();
			Console << U"Response: {}"_fmt(response.getStatusLine());

			if (response.isOK())
			{
				Array<std::pair<String, double>> scoreboard;
				ParseScoreboardJson(scoreboardGetTask->getAsJSON(), scoreboard);
				scoreboardTable = CreateScoreboardTable(scoreboard);
			}

			scoreboardGetTask.reset();
		}

		//// UI ////

		bool uiEnabled = not scoreboardGetTask.has_value() && not scoreboardPostTask.has_value();

		SimpleGUI::TextBox(usernameInput, { 10, 10 }, 200, 20, uiEnabled);

		if (SimpleGUI::Button(U"追加 \U000F0415", { 215, 10 }, unspecified, uiEnabled))
		{
			String username = usernameInput.text;
			double score = RandomClosed(0.0, 100.0);

			// 新しいスコアを送信する
			scoreboardPostTask = PushScoreAsync(username, score);
		}

		Line{ 330, 10, 330, 45 }.draw(Palette::Black);

		if (SimpleGUI::Button(U"更新 \U000F0453", { 340, 10 }, unspecified, uiEnabled))
		{
			// スコアボードを更新する
			scoreboardGetTask = GetScoreboardJsonAsync();
		}

		scoreboardTable.draw({ 10, 55 });
	}
}
