const Spreadsheet = SpreadsheetApp.getActiveSpreadsheet();
const LeaderboardSheet = Spreadsheet.getSheets()[0];
const QuerySheet = Spreadsheet.getSheets()[1];
const ResultSheet = Spreadsheet.getSheets()[2];

const DEFAULT_SCORE = 0; // スコアの読み取りに失敗した場合に設定する値
const TIMEZONE = "Asia/Tokyo"; // タイムスタンプのタイムゾーン
const ANONYMOUS_NAME = "[匿名]"; // 匿名ユーザーの表示名
const RECORD_LIMIT = 100; // レコードを取得できる上限数 (サーバー負荷を抑えるため)

/**
 * シート「検索クエリ」のセルB2に値を設定
 * @param {string} value クエリ
 */
function setSearchQuery(value)
{
  QuerySheet.getRange(2, 2).setValue(value);
}

/**
 * 値をnumber型に変換し、失敗した場合はDEFAULT_SCOREを返す
 * @param {any} value ユーザー名
 */
function parseScore(value)
{
  if (typeof(value) === "number" &&
    isFinite(value))
  {
    return value;
  }

  if (typeof(value) === "string" &&
    value.match(/^[+-]?(\d*[.])?\d+$/g))
  {
    return parseFloat(value);
  }

  return DEFAULT_SCORE;
}

/**
 * スコアを追加
 * @param {string} username ユーザー名
 * @param {number} score スコア
 * @param {any} data 追加データ
 */
function pushScore(username, score, data=undefined)
{
  // 日本時間、現在のタイムスタンプを作成
  let timestamp = Utilities.formatDate(new Date(), TIMEZONE, 'yyyy-MM-dd HH:mm:ss');

  // ユーザー名の型が文字列型ではないか、
  // 空の場合は匿名にする
  if (typeof(username) !== "string" ||
    username.length == 0)
  {
    username = ANONYMOUS_NAME;
  }

  // スコアを読み込む
  score = parseScore(score);

  // スコアデータをシートに追加
  LeaderboardSheet.appendRow([
    timestamp, username, score, JSON.stringify(data)
  ]);
}

/**
 * 上位のスコアを取得
 * @param {number} limit 取得最大個数
 * @return {{ username: string, score: number }[]} スコアの一覧
 */
function getTopScores(limit = null)
{
  if (typeof(username) !== "number" ||
    limit > RECORD_LIMIT)
  {
    limit = RECORD_LIMIT;
  }

  // クエリ文字列を設定
  setSearchQuery(`select B, C, D where A is not null order by C desc limit ${limit}`);

  let srcData = ResultSheet.getDataRange().getValues();
  let labelList = srcData.shift();
  
  let dstData = srcData;

  // データの形式を変換
  dstData = dstData.map(row => {
    let obj = { };
    row.forEach((value, i) => {
      obj[labelList[i]] = value;
    });
    return obj;
  });

  // 特定のキーの型を変換
  dstData = dstData.map(record => {
    if ("username" in record)
    {
      record.username = String(record.username);
    }
    if ("score" in record)
    {
      record.score = parseScore(record.score);
    }
    if ("data" in record)
    {
      try
      {
        record.data = JSON.parse(record.data);
      }
      catch(SyntaxError)
      {
        record.data = undefined;
      }
    }
    return record;
  });

  return dstData;
}

/**
 * GETリクエスト処理
 */
function doGet(e)
{
  // 取得個数
  let count = null;

  if ("count" in e.parameter &&
    e.parameter.count.match(/^\d+$/g))
  {
    count = parseInt(e.parameter.count, 10);
  }

  // リーダーボードを取得
  let leaderboard = getTopScores(count);

  // リーダーボードをJSONで送信
  return ContentService.createTextOutput()
    .setContent(JSON.stringify(leaderboard))
    .setMimeType(ContentService.MimeType.JSON);
}

/**
 * POSTリクエスト処理
 */
function doPost(e)
{
  // エラーメッセージ
  let errorMessages = [];

  // 追加データ
  let parsedData = undefined;

  // クエリのバリデーションと例外処理
  if (!("username" in e.parameter))
  {
    errorMessages.push("パラメータusernameが存在しません");
  }
  if (!("score" in e.parameter))
  {
    errorMessages.push("パラメータscoreが存在しません");
  }
  if ("data" in e.parameter)
  {
    try
    {
      parsedData = JSON.parse(e.parameter.data);
    }
    catch (SyntaxError)
    {
      errorMessages.push("パラメータdataの文法が間違っています");
    }
  }
  if (errorMessages.length > 0)
  {
    // 異常終了
    throw Error(errorMessages.join("\n"));
  }

  // スコアを追加
  pushScore(e.parameter.username, e.parameter.score, parsedData);

  // 正常終了
  return ContentService.createTextOutput("OK");
}



function pushScoreTest1()
{
  const users = [ "太郎", "一郎", "二郎", "三郎" ];
  let user = users[Math.floor(Math.random() * users.length)];
  pushScore(user, Math.random() * 100);
}

function pushScoreTest2()
{
  pushScore("", Math.random() * 100);
}

function pushScoreTest3()
{
  const users = [ "太郎", "一郎", "二郎", "三郎" ];
  let user = users[Math.floor(Math.random() * users.length)];
  pushScore(user, Math.random() * 100, {hoge1:"test"});
}

function getTopScoresTest()
{
  Logger.log(JSON.stringify(getTopScores()));
  Logger.log(JSON.stringify(getTopScores(3)));
}
