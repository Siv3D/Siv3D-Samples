# include <Siv3D.hpp>

/// @brief レコード
struct Record
{
	/// @brief ユーザー名
	String userName;

	/// @brief スコア
	double score;

	/// @brief 追加の情報（このサンプルでは特に使いません）
	JSON data;
};

/// @brief 有効なレコードかどうかをチェックします。
/// @param value レコードが格納された JSON
/// @return 有効なレコードなら true, そうでなければ false
bool IsValidRecord(const JSON& value)
{
	return (value.isObject()
		&& value.hasElement(U"username")
		&& value.hasElement(U"score")
		&& value[U"username"].isString()
		&& value[U"score"].isNumber());
}

/// @brief JSON データをリーダーボードとして読み込みます。
/// @param json JSON データ
/// @param dst 更新するリーダーボード
/// @remark 読み込みに失敗した場合、dst は更新されません。
/// @return 読み込みに成功したら true, 失敗したら false
bool ReadLeaderboard(const JSON& json, Array<Record>& dst)
{
	if (not json.isArray())
	{
		return false;
	}

	Array<Record> leaderboard;

	for (auto&& [key, value] : json)
	{
		if (not IsValidRecord(value))
		{
			continue;
		}

		Record record;
		record.userName = value[U"username"].get<String>();
		record.score = value[U"score"].get<double>();

		if (value.contains(U"data"))
		{
			record.data = value[U"data"];
		}

		leaderboard << std::move(record);
	}

	dst = std::move(leaderboard);
	return true;
}

/// @brief リーダーボードから SimpleTable を作成します。
/// @param leaderboard リーダーボード
/// @return SimpleTable
SimpleTable ToTable(const Array<Record>& leaderboard)
{
	SimpleTable table{ { 100, 260, 140 }};

	// ヘッダー行を追加する
	table.push_back_row({ U"Rank", U"Player Name", U"Score" }, { 0, 0, 0 });
	table.setRowBackgroundColor(0, ColorF{ 0.92 });

	// 順位
	int32 rank = 1;

	// リーダーボードの内容を追加する
	for (auto& record : leaderboard)
	{
		table.push_back_row({ Format(rank++), record.userName, Format(record.score) });
	}

	return table;
}

/// @brief サーバからリーダーボードを取得するタスクを作成します。
/// @param url サーバの URL
/// @param count 取得上限数
/// @return タスク
AsyncHTTPTask CreateGetTask(const URLView url, int32 count = 10)
{
	// GET リクエストの URL を作成する
	const URL requestURL = U"{}?count={}"_fmt(url, count);

	return SimpleHTTP::GetAsync(requestURL, {});
}

/// @brief サーバにスコアを送信するタスクを作成します。
/// @param url サーバの URL
/// @param userName ユーザー名
/// @param score スコア
/// @param additionalData 追加の情報
/// @return タスク
AsyncHTTPTask CreatePostTask(const URLView url, const StringView userName, double score, JSON additionalData = JSON::Invalid())
{
	// POST リクエストの URL を作成する
	URL requestURL = U"{}?username={}&score={}"_fmt(url, PercentEncode(userName), PercentEncode(Format(score)));

	if (additionalData)
	{
		requestURL += (U"&data=" + PercentEncode(additionalData.formatMinimum()));
	}

	const HashTable<String, String> headers =
	{
		{ U"Content-Type", U"application/x-www-form-urlencoded; charset=UTF-8" }
	};

	return SimpleHTTP::PostAsync(requestURL, headers, nullptr, 0);
}

/// @brief ランダムなスコアを返します。
/// @return ランダムなスコア
double MakeRandomScore()
{
	return (Random(10000) / 100.0);
}

/// @brief ランダムなユーザー名を作成します。
/// @return ランダムなユーザー名
String MakeRandomUserName()
{
	static const Array<String> words1 =
	{
		U"Blue", U"Red", U"Green", U"Silver", U"Gold",
		U"Happy", U"Angry", U"Sad", U"Exciting", U"Scary",
		U"Big", U"Small", U"Large", U"Tiny", U"Short",
	};

	static const Array<String> words2 =
	{
		U"Lion", U"Dragon", U"Tiger", U"Eagle", U"Shark",
		U"Pizza", U"Curry", U"Ramen", U"Sushi", U"Salad",
		U"Cat", U"Dog", U"Mouse", U"Rabbit", U"Fox",
	};

	return (U"{} {} {:0>4}"_fmt(words1.choice(), words2.choice(), Random(9999)));
}

void Main()
{
	// Google Apps Script の URL（サンプル用の URL. 定期的に記録がクリアされます）
	// 実行ファイルに URL が直接埋め込まれるのを防ぐため、SIV3D_OBFUSCATE() で URL を難読化
	const std::string url{ SIV3D_OBFUSCATE("https://script.google.com/macros/s/AKfycbwyGtLLG628VDu_-0wTZDHVyEdbja0xgWFMoZfc_tjxEfYn69QrZgTDyHS1t2gbffEJ/exec") };
	const URL LeaderboardURL = Unicode::Widen(url);

	Scene::SetBackground(ColorF{ 0.6, 0.8, 0.7 });

	const Font font{ FontMethod::MSDF, 48 };

	// リーダーボードを表示するテーブル
	SimpleTable table;

	// リーダーボードを取得するタスク
	Optional<AsyncHTTPTask> leaderboardGetTask = CreateGetTask(LeaderboardURL);

	// スコアを送信するタスク
	Optional<AsyncHTTPTask> scorePostTask;

	// 自身のユーザ名
	String userName = MakeRandomUserName();

	// 自身のスコア
	double score = MakeRandomScore();

	// 最後にリーダーボードを取得した時刻
	DateTime lastUpdateTime{ 2023, 1, 1 };

	// スコアを送信したか
	bool isScorePosted = false;

	while (System::Update())
	{
		// 通信が完了しているか
		const bool isReady = (not leaderboardGetTask) && (not scorePostTask);

		// 自身のユーザー名を更新する
		if (SimpleGUI::Button(U"\U000F0004 {}"_fmt(userName), Vec2{ 40, 40 }, 330))
		{
			userName = MakeRandomUserName();
			isScorePosted = false;
		}

		// 自身のスコアを更新する
		if (SimpleGUI::Button(U"\U000F0AE2 {}"_fmt(score), Vec2{ 384, 40 }, 160))
		{
			score = MakeRandomScore();
			isScorePosted = false;
		}

		// 現在のスコアを送信する
		if (SimpleGUI::Button(U"\U000F0415 Register", { 560, 40 }, 160, (isReady && (not isScorePosted))))
		{
			scorePostTask = CreatePostTask(LeaderboardURL, userName, score);
		}

		// リーダーボードを更新する
		if (SimpleGUI::Button(U"\U000F0453 Refresh", { 560, 100 }, 160, isReady))
		{
			leaderboardGetTask = CreateGetTask(LeaderboardURL);
		}

		// リーダーボードの更新時刻を表示する
		font(U"Last updated:\n{}"_fmt(lastUpdateTime)).draw(12, 560, 140, ColorF{ 0.25 });

		// スコア送信処理が完了したら
		if (scorePostTask && scorePostTask->isReady())
		{
			if (const auto response = scorePostTask->getResponse();
				response.isOK())
			{
				// スコアを送信済みにし、再送信できないようにする
				isScorePosted = true;

				// リーダーボードを更新する
				leaderboardGetTask = CreateGetTask(LeaderboardURL);
			}
			else
			{
				Print << U"Failed to submit the score.";
			}

			scorePostTask.reset();
		}

		// リーダーボード取得処理が完了したら
		if (leaderboardGetTask && leaderboardGetTask->isReady())
		{
			if (const auto response = leaderboardGetTask->getResponse();
				response.isOK())
			{
				Array<Record> leaderboard;

				if (ReadLeaderboard(leaderboardGetTask->getAsJSON(), leaderboard))
				{
					// リーダーボードを表示するテーブルの内容を更新する
					table = ToTable(leaderboard);

					// 最後にリーダーボードを取得した時刻を更新する
					lastUpdateTime = DateTime::Now();
				}
				else
				{
					Print << U"Failed to read the leaderboard.";
				}
			}

			leaderboardGetTask.reset();
		}

		// リーダーボードを描画する
		if (table)
		{
			table.draw({ 40, 100 });
		}
		else
		{
			// リーダーボードが空の場合は、ロード中であることを示すアニメーションを描画する
			Circle{ 292, 260, 80 }.drawArc((Scene::Time() * 90_deg), 300_deg, 10, 0);
		}
	}
}
