# Siv3D-Samples
誰でも投稿できる Siv3D サンプル集。試験運用中です。

## 一覧

| <a href="Samples/Minesweeper"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/Minesweeper/Screenshot/3.png" width="250px"></a> | <a href="Samples/TopDownShooterP2"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/TopDownShooterP2/Screenshot/2.png" width="250px"></a> | <a href="Samples/FocusArrows"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/FocusArrows/Screenshot/3.png" width="250px"></a> |
|:--:|:--:|:--:|
| マインスイーパー | 見下ろし型 2D シューティング | カーソルを注目する矢印 |
| Minesweeper | Top-down 2D Shooter | Focus Arrows |

| <a href="Samples/TabSample"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/TabSample/Screenshot/2.png" width="250px"></a> |
|:--:|
| タブサンプル |
| Tab Samples |


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


## README テンプレート

```
# タイトル日本語 | タイトル英語

|               |                                              |
|:--------------|:---------------------------------------------|
| Author        | 作者                                         |
| Affiliation   | 所属（空欄可）                               |
| Siv3D Version | v0.6.5 ← 動作を確認した Siv3D バージョン              |
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
