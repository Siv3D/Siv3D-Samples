# Siv3D-Samples
誰でも投稿できる Siv3D サンプル集。試験運用中です。

## 一覧

| <a href="Samples/Minesweeper"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/Minesweeper/Screenshot/3.png" width="250px"></a> | <a href="Samples/TopDownShooterP2"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/TopDownShooterP2/Screenshot/2.png" width="250px"></a> | <a href="Samples/FocusArrows"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/FocusArrows/Screenshot/3.png" width="250px"></a> |
|:--:|:--:|:--:|
| マインスイーパー | 見下ろし型 2D シューティング | カーソルを注目する矢印 |
| Minesweeper | Top-down 2D Shooter | Focus Arrows |

| <a href="Samples/TabSample"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/TabSample/Screenshot/2.png" width="250px"></a> | <a href="Samples/HPBar"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/HPBar/Screenshot/2.png" width="250px"></a> | <a href="Samples/SimpleOthelloAI"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/SimpleOthelloAI/Screenshot/2.png" width="250px"></a> |
|:--:|:--:|:--:|
| タブ | HP バー | AI オセロ 🏆 |
| Tabs | HP bar | AI Othello |

| <a href="Samples/AudioCrossfade"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/AudioCrossfade/Screenshot/2.png" width="250px"></a> | <a href="Samples/Klondike"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/Klondike/Screenshot/2.png" width="250px"></a> | <a href="Samples/RandomWalkKaleidoscope"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/RandomWalkKaleidoscope/Screenshot/2.png" width="250px"></a> |
|:--:|:--:|:--:|
| BGM クロスフェード 🏆 | クロンダイク 🏆 | 万華鏡ランダムウォーク 🏆 |
| Audio Crossfade | Klondike | Random Walk Kaleidoscope |

| <a href="Samples/GPT3Story"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/GPT3Story/Screenshot/1.png" width="250px"></a> | <a href="Samples/IsometricView"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/IsometricView/Screenshot/1.png" width="250px"></a> | <a href="Samples/AutoTiles"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/AutoTiles/Screenshot/1.png" width="250px"></a> |
|:--:|:--:|:--:|
| AI による物語生成 | クォータービュー | オートタイル |
| AI Story Generator | Isometric View | Auto Tiles |

| <a href="Samples/Leaderboard"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/Leaderboard/Screenshot/1.png" width="250px"></a> |
|:--:|
| オンライン リーダーボード 🏆 |
| Online Leaderboard |


🏆 マークはユーザ投稿サンプルです。


## 投稿方法

ゲーム名のフォルダに

- `README.md`
- `Main.cpp`
- `Screenshot/`
- その他必要なファイル（`engine` / `example` は除く）

を含め、`Samples/` 以下に追加する pull-request を送ってください。[`Samples/Minesweeper`](Samples/Minesweeper) が参考になります。  

- 投稿はレビューののち採否を決定します
- ゲーム・アプリ作品、開発に役立つサンプル、テクニック紹介などを募集します
- コードの規模は、改造や再利用がしやすい数百行程度が理想的ですが、厳しい縛りはありません
- 採用された作品は `CC0-1.0 license` として扱います
- 掲載にあたってコードのリファクタリングや README の修正を行います
- 掲載後の修正も歓迎です


## README テンプレート

```
# タイトル日本語 | タイトル英語

|               |                                              |
|:--------------|:---------------------------------------------|
| Author        | 作者                                         |
| Affiliation   | 所属（空欄可）                               |
| Siv3D Version | v0.6.11 ← 動作を確認した Siv3D バージョン              |
| Platform      | Windows, macOS, Linux, Web のうち、動作を確認したもの |

## 説明 | Description

実装や技術に関する説明を書きます。

## 遊び方 | How to Play

- 箇条書きで
- 遊び方を
- 簡潔に
- 説明します

## スクリーンショット | Screenshots

![](Screenshot/1.png)
```
