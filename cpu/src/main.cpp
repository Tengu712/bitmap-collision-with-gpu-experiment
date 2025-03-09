#include "../../common/common.hpp"

#include <array>
#include <iostream>
#include <Windows.h>

class Entity final: public EntityBase {
public:
	Entity(float x, float y, float r, float spd, float dir): EntityBase(x, y, r, spd, dir) {}
	bool isHit(const Entity &opponent) const {
		const auto dx = _x - opponent._x;
		const auto dy = _y - opponent._y;
		const auto rr = _r + opponent._r;
		return dx * dx + dy * dy < rr * rr;
	}
};

class Scene final: public SceneBase<Entity> {
public:
	explicit Scene(size_t entityCount): SceneBase(entityCount) {}
	void update() {
		for (auto &n: _entities1) {
			n.update();
		}
		for (auto &n: _entities2) {
			n.update();
			for (auto &m: _entities1) {
				if (n.isHit(m)) {
					SceneBase::incrementHitCount();
				}
			}
		}
	}
};

int main() {
	constexpr std::array<size_t, 7> entityCounts{100, 500, 1000, 2000, 3000, 4000, 5000};
	for (auto entityCount: entityCounts) {
		Scene scene(entityCount);

		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		LARGE_INTEGER before;
		QueryPerformanceCounter(&before);

		for (int i = 0; i < 1000; ++i) {
			scene.update();
		}

		LARGE_INTEGER after;
		QueryPerformanceCounter(&after);

		std::cout
			<< entityCount
			<< " "
			<< (static_cast<double>(after.QuadPart - before.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart))
			<< " "
			<< scene.getHitCount()
			<< std::endl;

		// NOTE: 念のため、CPUを冷ますために2秒待つ。
		Sleep(2000);
	}
}
