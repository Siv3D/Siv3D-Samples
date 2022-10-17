# Siv3D-Samples
ユーザも投稿できる Siv3D サンプル集。試験運用中です。

## 一覧

|    |
|:--:|
|<a href="Samples/Minesweeper"><img src="https://raw.githubusercontent.com/Siv3D/Siv3D-Samples/main/Samples/Minesweeper/Screenshot/3.png" width="240px"></a><br>マインスイーパー / Minesweeper |

## 投稿方法

ゲーム名のフォルダに

- `README.md`
- `Main.cpp`
- `Screenshot/`
- その他必要なファイル (engine / example を除く)

を含め、`Samples/` 以下に追加する pull-request を送ってください。`Samples/Minesweeper` が参考になります。  

- 投稿はレビューののち採否を決定します
- `Main.cpp` に収まる、数百行程度の完結した作品や、開発に役立つサンプルであることが好ましいです 
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

## 遊び方 | How to Play

- 箇条書きで
- 遊び方を
- 簡潔に
- 説明します

## スクリーンショット | Screenshots

![](Screenshot/1.png)
```
