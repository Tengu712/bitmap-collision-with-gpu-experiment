#pragma once

#include "constant.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#undef max
#undef min

class EntityBase {
protected:
	float _x, _y, _r, _spd, _dir;

public:
	EntityBase(float x, float y, float r, float spd, float dir): _x(x), _y(y), _r(r), _spd(spd), _dir(dir) {}
	virtual ~EntityBase() = default;
	inline void update() {
		_x += _spd * std::cosf(_dir);
		_y += _spd * std::sinf(_dir);
		if (_x < 0.0f || _x > WIDTH_FLOAT) {
			_dir = PI - _dir;
			_x = std::max(std::min(_x, WIDTH_FLOAT), 0.0f);
		}
		if (_y < 0.0f || _y > HEIGHT_FLOAT) {
			_dir += PI;
			_y = std::max(std::min(_y, HEIGHT_FLOAT), 0.0f);
		}
	}
};

template<typename T>
class SceneBase {
protected:
	unsigned long long _hitCount;
	std::vector<T> _entities1;
	std::vector<T> _entities2;

public:
	explicit SceneBase(size_t entityCount): _hitCount(0) {
		_entities1.reserve(entityCount);
		_entities2.reserve(entityCount);
		const auto dx = WIDTH_FLOAT / static_cast<float>(entityCount);
		for (int i = 0; i < entityCount; ++i) {
			_entities1.emplace_back(i * dx + dx / 2.0f,          10.0f, 5.0f, 2.5f, (i * 10.0f) * PI / 180.0f);
			_entities2.emplace_back(i * dx + dx / 2.0f, HEIGHT - 10.0f, 5.0f, 2.5f, (i * 10.0f) * PI / 180.0f);
		}
	}
	virtual ~SceneBase() = default;
	inline void incrementHitCount() {
		_hitCount += 1;
	}
	inline unsigned long long getHitCount() const {
		return _hitCount;
	}
};
