# オセロ AI | Othello AI

|               |                                  |
| :------------ | :------------------------------- |
| Author        | [山名琢翔](https://nyanyan.dev/) |
| Affiliation   | 筑波大学                         |
| Siv3D Version | v0.6.5                           |
| Platform      | Windows, macOS, Linux, Web       |

## 説明 | Description

コンパクトな行数にも関わらず結構強いオセロ AI ライブラリと、それを用いてオセロ対局を行えるアプリを実装しました。

### 概要

アルゴリズムは Nega-Alpha 法、評価関数はマスの重みによる評価を使用しています。ボードの実装にはビットボードを使用しています。定石データは使用していません。

### アルゴリズム

このオセロ AI では Nega-Alpha 法を使用していますが、move ordering は使用していません。また、置換表も使用していません。

### 評価関数

評価関数は最終石差（その盤面から双方最善を尽くしたら最終的にどれだけの石差でどちらが勝つか）を目標として山登り法で調整しました。調整に使ったコードは[こちら](https://github.com/Nyanyan/Siv3D_OthelloAI/blob/main/evaluation/eval.cpp)です。

### （宣伝）世界最強のオセロ AI

このサンプルとは別で、自作の世界最強オセロ AI を、OpenSiv3D を使用して GUI から動かせるようにしました。オセロ AI やアプリとしての完成度は本サンプルよりも格段に高いです。
- [Egaroucid](https://www.egaroucid-app.nyanyan.dev/)

## 遊び方 | How to Play

- 「先手 (黒) で対局開始」か「後手 (白) で対局開始」ボタンを押して対局を開始します
- 「あなたの手番」と表示されているときは、白い枠で囲われたマスに打つことができます。マスをクリックすると着手できます
- AI に勝てるよう頑張ってください。そこそこ強いです
- AI の先読み手数（AI の性能）はコード上で調整できます

## スクリーンショット | Screenshots

![](Screenshot/1.png)

![](Screenshot/2.png)

![](Screenshot/3.png)

![](Screenshot/4.png)
