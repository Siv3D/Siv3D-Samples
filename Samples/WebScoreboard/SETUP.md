# リーダーボードの作成方法

## スプレッドシートの作成

Googleアカウントにログインし、下記リンクからスプレッドシートのコピーを作成する

| [Leaderboardテンプレート](https://docs.google.com/spreadsheets/d/1IHG3NuneuxWctajEr_-44pAU8F2YZDZbRbDODK7w37U/copy) |
|-|

## APIの公開

1. コピーしたスプレッドシートを開き、画面上のメニューから`拡張機能`→`Apps Script`と進みGASの編集画面を表示

2. 画面右上にある青い`デプロイ`ボタンをクリック→`新しいデプロイ`

3. `新しいデプロイ`ダイアログが表示されたら、設定を以下の通りに変更してデプロイ
    - 次のユーザーとして実行：`自分`
    - アクセスできるユーザー：`全員`
  ![](Screenshot/3.png)

4. アクセス権が要求される場合があるので、その場合は許可をする

5. デプロイが完了したら、ウェブアプリのURLをコピー
![](Screenshot/4.png)

## URLの置き換え

`Main.cpp`の`LeaderboardURL`の値をコピーしたURLに置き換える

```cpp
void Main()
{
	// Google Apps Script の URL
	constexpr URLView LeaderboardURL = U"<ここにURLを貼り付け>";

  ...
}
```
