# include <Siv3D.hpp> // OpenSiv3D v0.6.5

/// @brief タイムスタンプ（秒）を表現する型
using TimestampSec = double;

/// @brief 「壁」の干渉フィルタ
constexpr P2Filter WallFilter{ .categoryBits = 0b0000'0000'0000'0001, .maskBits = 0b1111'1111'1111'1111 };

/// @brief 「味方の弾」の干渉フィルタ（味方の弾、味方ユニットとは干渉しない）
constexpr P2Filter FriendBulletFilter{ .categoryBits = 0b0000'0000'0000'0010, .maskBits = 0b1111'1111'1111'1001 };

/// @brief 「味方ユニット」の干渉フィルタ（味方ユニット、敵ユニットとは干渉しない）
constexpr P2Filter FriendUnitFilter{ .categoryBits = 0b0000'0000'0000'0100, .maskBits = 0b1111'1111'1111'0011 };

/// @brief 「敵ユニット」の干渉フィルタ（味方ユニット、敵ユニットとは干渉しない）
constexpr P2Filter EnemyUnitFilter{ .categoryBits = 0b0000'0000'0000'1000, .maskBits = 0b1111'1111'1111'0011 };

[[nodiscard]]
constexpr int32 ImpulseToDamage(double impulse)
{
	return static_cast<int32>(impulse * 100);
}

/// @brief 衝突イベント
struct CollisionEvent
{
	/// @brief 衝突した物体の P2BodyID
	P2BodyID a;

	/// @brief 衝突した物体の P2BodyID
	P2BodyID b;

	/// @brief 衝突位置
	Vec2 pos;

	/// @brief 衝突時の法線方向の力
	double normalImpulse = 0.0;

	/// @brief 衝突時の接線方向の力
	double tangentImpulse = 0.0;

	/// @brief 衝突が発生したタイムスタンプ
	TimestampSec timestamp;
};

/// @brief 敵ユニット
struct Enemy
{
	static constexpr double Radius = 40.0;

	P2Body body;

	int32 hp;

	int32 maxHP;

	bool move;

	void damage(double normalImpulse)
	{
		hp = Max((hp - ImpulseToDamage(normalImpulse)), 0);
	}

	void draw(const Font& font)
	{
		const Vec2 pos = body.getPos();

		Circle{ pos, Radius }.draw(Palette::Magenta);

		font(U"{}/{}"_fmt(hp, maxHP)).drawAt(14, pos);
	}
};

/// @brief 衝突エフェクトクラス
struct RingEffect : IEffect
{
	Vec2 m_pos;

	double m_normalImpulse;

	ColorF m_color;

	explicit RingEffect(const Vec2& pos, double normalImpulse)
		: m_pos{ pos }
		, m_normalImpulse{ normalImpulse }
		, m_color{ RandomColorF() } {}

	bool update(double t) override
	{
		// 時間に応じて大きくなる輪
		Circle{ m_pos, (15 + t * 80) }.drawFrame(12 * (0.5 - t), m_color);

		FontAsset(U"BoldFont")(ImpulseToDamage(m_normalImpulse))
			.drawAt(TextStyle::Outline(0.2, ColorF{ 0.1, (1.0 - t * 2) }), m_pos + Vec2{ 20, -20 - t * 120 }, ColorF{ 1.0, (1.0 - t * 2) });

		// 0.5 秒未満なら継続
		return (t < 0.5);
	}
};

/// @brief すべての弾の管理クラス
class BulletList
{
public:

	/// @brief 弾を発射する
	/// @param world 物理演算ワールド
	/// @param from 発射位置
	/// @param velocity 発射速度
	/// @param density 弾の密度 (kg / m^2)
	/// @param timeStamp 発射時刻
	void fire(P2World& world, const Vec2& from, const Vec2& velocity, double density, TimestampSec timestamp)
	{
		P2Body body = world.createCircle(P2Dynamic, from, 5, P2Material{ .density = density }, FriendBulletFilter).setVelocity(velocity);
		// body.setBullet(true); // 高速な弾のすり抜けを防止できる (OpenSiv3D v0.6.6 以降）
		m_bullets << body;
		m_bulletSet.emplace(body.id(), timestamp);
	}

	/// @brief すべての弾に空気抵抗相当の力を与える
	/// @param dt タイムステップ
	void applyAirResistance(double dt)
	{
		for (auto& bullet : m_bullets)
		{
			// 速さに比例した空気抵抗
			bullet.applyLinearImpulse(-bullet.getVelocity() * dt * AirResistance);
		}
	}

	/// @brief 指定した P2BodyID の弾を削除する
	/// @param id 削除する弾の P2BodyID
	void remove(P2BodyID id)
	{
		if (not m_bulletSet.contains(id))
		{
			return;
		}

		for (auto it = m_bullets.begin(); it != m_bullets.end(); ++it)
		{
			if (it->id() == id)
			{
				m_bulletSet.erase(id);
				m_bullets.erase(it);
				break;
			}
		}
	}

	/// @brief 指定した領域外の弾を削除する
	/// @param bounds 領域
	void removeOutOfBounds(const RectF& bounds)
	{
		for (auto it = m_bullets.begin(); it != m_bullets.end();)
		{
			if (not it->getPos().intersects(bounds))
			{
				m_bulletSet.erase(it->id());
				it = m_bullets.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	/// @brief 指定したタイムスタンプ未満の弾を削除する
	/// @param t タイムスタンプ
	void removeOutDatad(TimestampSec t)
	{
		for (auto it = m_bullets.begin(); it != m_bullets.end();)
		{
			if (m_bulletSet[it->id()] < t)
			{
				m_bulletSet.erase(it->id());
				it = m_bullets.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	/// @brief すべての弾を描画する
	void draw() const
	{
		for (const auto& bullet : m_bullets)
		{
			const Vec2 pos = bullet.getPos();

			Circle{ pos, 5 }.draw();

			if (5.0 <= bullet.shape(0).getDensity())
			{
				Circle{ pos, 8 }.drawFrame(1);
			}
		}
	}

	/// @brief 指定した P2Body が弾であるかを返す。
	/// @param id P2BodyID
	/// @return 指定した P2Body が弾である場合 true, それ以外の倍は false
	bool isBullet(P2BodyID id) const
	{
		return m_bulletSet.contains(id);
	}

	/// @brief 状態を Print する
	void showStats() const
	{
		assert(m_bullets.size() == m_bulletSet.size());
		Print << U"active bullets: {}"_fmt(m_bullets.size());
	}

private:

	// 空気抵抗の強さ
	static constexpr double AirResistance = 0.002;

	// アクティブな弾
	Array<P2Body> m_bullets;

	// アクティブな弾の P2BodyID 一覧
	HashTable<P2BodyID, TimestampSec> m_bulletSet;
};

P2Body AddWall(P2World& world, const RectF& rect)
{
	return world.createRect(P2Static, rect.center(), rect.size, {}, WallFilter);
}

void Main()
{
	// ウィンドウを 1280x720 にリサイズする
	Window::Resize(1280, 720);

	FontAsset::Register(U"BoldFont", FontMethod::MSDF, 32, Typeface::Bold);
	const Font& boldFont = FontAsset(U"BoldFont");

	// この範囲外に飛んだ弾は削除される
	const RectF GameBounds{ -400, -250, 800, 500 };

	// 壁
	const Rect Wall1Rect{ -300, -210, 600, 20 };
	const Rect Wall2Rect{ -300, 190, 600, 20 };
	const Circle FriendCircle{ -300, -100, 40 };

	// 2D 物理演算のシミュレーションステップ（秒）
	constexpr double StepSec = (1.0 / 200.0);

	// 2D 物理演算のシミュレーション蓄積時間（秒）
	double accumulatorSec = 0.0;

	// 2D 物理演算のワールド
	P2World world{ 0.0 }; // 重力設定 0

	// 壁
	const P2Body wall1Body = AddWall(world, Wall1Rect);
	const P2Body wall2Body = AddWall(world, Wall2Rect);

	// 味方ユニット
	P2Body friendBody = world.createCircle(P2Static, FriendCircle.center, FriendCircle.r, {}, FriendUnitFilter);

	// 敵ユニット
	HashTable<P2BodyID, Enemy> enemies;
	{
		{
			Enemy e{ world.createCircle(P2Kinematic, Vec2{ 200, 100 }, Enemy::Radius, {}, EnemyUnitFilter), 3000, 3000, false };
			enemies.emplace(e.body.id(), e);
		}

		{
			Enemy e{ world.createCircle(P2Kinematic, Vec2{ 300, 100 }, Enemy::Radius, {}, EnemyUnitFilter), 3000, 3000, true };
			enemies.emplace(e.body.id(), e);
		}
	}

	// 2D カメラ。初期中心座標: (0, 0), 拡大倍率: 1.0, 手動操作なし
	Camera2D camera{ Vec2{ 0, 0 }, 1.0, CameraControl::None_ };

	Circle player{ 0, 0, 10 };

	BulletList bulletList;

	// ゲーム時刻
	TimestampSec gameClock = 0.0;

	Effect effect;

	while (System::Update())
	{
		////////////////////////////////
		//
		//	状態更新
		//
		////////////////////////////////

		// 自分の向き (rad)
		const double angle = (Cursor::PosF() - Scene::CenterF()).getAngle();

		// キーを押すと弾を発射する
		if (KeyW.down() || KeyS.down())
		{
			const double speed = 500.0;
			const Vec2 velocity = Circular{ speed, angle }; // 初速
			const double density = (KeyW.down() ? 1.0 : 5.0); // 弾の密度（威力に影響）
			bulletList.fire(world, player.center, velocity, density, gameClock);
		}

		// 現在のフレームでの弾に関する衝突イベント
		Array<CollisionEvent> bulletCollisionEvents;

		for (accumulatorSec += Scene::DeltaTime(); (StepSec <= accumulatorSec); accumulatorSec -= StepSec)
		{
			gameClock += StepSec;

			// すべての弾に空気抵抗相当の力を与える
			bulletList.applyAirResistance(StepSec);

			// enemy2 を移動させる
			for (auto&& [id, enemy] : enemies)
			{
				if (enemy.move)
				{
					enemy.body.setPos(enemy.body.getPos().x, Math::Sin(gameClock * 45_deg) * 100);
				}
			}

			// 2D 物理演算のワールドを更新する
			world.update(StepSec);

			// 接触イベントを取得する
			for (auto&& [pair, collision] : world.getCollisions())
			{
				// 弾が関わらない接触はスキップする
				if ((not bulletList.isBullet(pair.a))
					&& (not bulletList.isBullet(pair.b)))
				{
					continue;
				}

				for (const auto& c : collision)
				{
					// 接触イベントを構築する
					const CollisionEvent ce
					{
						.a = pair.a,
						.b = pair.b,
						.pos = c.point,
						.normalImpulse = c.normalImpulse,
						.tangentImpulse = Abs(c.tangentImpulse),
						.timestamp = gameClock
					};

					// 接触イベント配列に追加する
					bulletCollisionEvents << ce;

					effect.add<RingEffect>(ce.pos, ce.normalImpulse);
				}

				// 接触した弾を削除する
				bulletList.remove(pair.a);
				bulletList.remove(pair.b);
			}
		}

		// 画面外に出た弾を削除する
		bulletList.removeOutOfBounds(GameBounds);

		// 発射から 5 秒以上経過した弾を削除する
		bulletList.removeOutDatad(gameClock - 5.0);

		// 敵ユニットに弾によるダメージを与える
		for (const auto& bulletCollisionEvent : bulletCollisionEvents)
		{
			if (auto it = enemies.find(bulletCollisionEvent.a);
				it != enemies.end())
			{
				it->second.damage(bulletCollisionEvent.normalImpulse);
			}

			if (auto it = enemies.find(bulletCollisionEvent.b);
					it != enemies.end())
			{
				it->second.damage(bulletCollisionEvent.normalImpulse);
			}
		}

		// HP が 0 以下の敵ユニットを削除する
		EraseNodes_if(enemies, [](const auto& e) { return (e.second.hp <= 0); });

		////////////////////////////////
		//
		//	描画
		//
		////////////////////////////////

		ClearPrint();
		Print << U"[W] 軽い弾を発射";
		Print << U"[S] 重い弾を発射";
		Print << U"gameClock: {:.2f}"_fmt(gameClock);
		bulletList.showStats();

		{
			// 2D カメラから Transformer2D を作成する
			const auto tr = camera.createTransformer();

			GameBounds.draw(ColorF{ 0.3 });
			Wall1Rect.draw();
			Wall2Rect.draw();
			bulletList.draw();
			player.draw(Palette::Yellow);
			Line{ player.center, Arg::angle = angle, 40.0 }.drawArrow(2, SizeF{ 10, 10 }, Palette::Yellow);
			friendBody.draw(Palette::Yellow);

			for (auto&& [id, enemy] : enemies)
			{
				enemy.draw(boldFont);
			}

			effect.update();
		}
	}
}
