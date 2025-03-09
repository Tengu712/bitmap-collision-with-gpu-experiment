#include "../../common/common.hpp"
#include "bitmap.hpp"
#include "core.hpp"
#include "render.hpp"
#include "window.hpp"

#include <iostream>

#undef min
#undef max

class Entity final: public EntityBase {
private:
	float _px, _py;

public:
	Entity(float x, float y, float r, float spd, float dir): EntityBase(x, y, r, spd, dir), _px(x), _py(y) {}
	void update() {
		_px = _x;
		_py = _y;
		EntityBase::update();
	}
	bool isHit(const BitmapManager &bmpMngr, unsigned int opponentGroup) {
		const int r = static_cast<int>(std::round(_r));
		const int x0 = static_cast<int>(std::round(_px));
		const int y0 = static_cast<int>(std::round(_py));
		int x = r;
		int y = 0;
		int f = -2 * r + 3;
		while (x >= y) {
			const auto b =
				   bmpMngr.check(x0 + x, y0 + y, opponentGroup)
				|| bmpMngr.check(x0 - x, y0 + y, opponentGroup)
				|| bmpMngr.check(x0 + x, y0 - y, opponentGroup)
				|| bmpMngr.check(x0 - x, y0 - y, opponentGroup)
				|| bmpMngr.check(x0 + y, y0 + x, opponentGroup)
				|| bmpMngr.check(x0 - y, y0 + x, opponentGroup)
				|| bmpMngr.check(x0 + y, y0 - x, opponentGroup)
				|| bmpMngr.check(x0 - y, y0 - x, opponentGroup);
			if (b) {
				return true;
			}
			if (f >= 0) {
				x -= 1;
				f -= 4 * x;
			}
			y += 1;
			f += 4 * y + 2;
		}
		return false;
	}
	void push(std::vector<EntityDataLayout> &data, DirectX::XMFLOAT4 mask) const {
		data.emplace_back(DirectX::XMFLOAT4(_x, _y, 0.0f, 0.0f), DirectX::XMFLOAT4(_r * 2.0f, _r * 2.0f, 1.0f, 1.0f), mask);
	}
};

class Scene final: public SceneBase<Entity> {
public:
	explicit Scene(size_t entityCount): SceneBase(entityCount) {}
	void update(Core &core, BitmapManager &bmpMngr, WindowManager &winMngr, Renderer &rndrr) {
		// 衝突判定ビットマップの描画が終わるまで待機
		core.wait();
		const auto frameIndex = core.getCurrentFrameIndex();

		// 衝突判定ビットマップの参照を開始
		bmpMngr.map(frameIndex);

		// 物体のデータを格納するvectorを作成
		std::vector<EntityDataLayout> data;
		data.reserve(_entities1.size() + _entities2.size());

		// 物体を更新
		for (auto &n: _entities1) {
			if (n.isHit(bmpMngr, 1)) {
				SceneBase<Entity>::incrementHitCount();
			}
			n.update();
			n.push(data, DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));
		}
		for (auto &n: _entities2) {
			if (n.isHit(bmpMngr, 0)) {
				SceneBase<Entity>::incrementHitCount();
			}
			n.update();
			n.push(data, DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f));
		}

		// 衝突判定ビットマップの参照を終了
		bmpMngr.unmap(frameIndex);

		// 衝突判定ビットマップに描画
		const auto &cmdList = core.getCurrentCommandList();
		rndrr.uploadEntities(frameIndex, data);
		bmpMngr.attach(cmdList, frameIndex);
		rndrr.draw(cmdList, frameIndex, static_cast<UINT>(data.size()));
		bmpMngr.detach(cmdList, frameIndex);

		// 画面に描画 (デバッグ用)
		winMngr.attach(cmdList);
		rndrr.draw(cmdList, frameIndex, static_cast<UINT>(data.size()));
		winMngr.detach(cmdList);

		// コマンド提出
		core.submit();

		// 画面にプレゼンテーション (デバッグ用)
		winMngr.present();

		// 次のフレームへ
		core.next();
	}
};

#ifdef WINDOW_RENDERING
int WINAPI WinMain(_In_ HINSTANCE inst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	constexpr std::array<size_t, 1> entityCounts{500};
#else
int main() {
	HINSTANCE inst = nullptr;
	constexpr std::array<size_t, 7> entityCounts{100, 500, 1000, 2000, 3000, 4000, 5000};
#endif
	for (auto entityCount: entityCounts) {
		Core core;
		BitmapManager bmpMngr(core.getDevice());
		WindowManager winMngr(inst, core.getDevice(), core.getQueue());
		Renderer rndrr(core.getDevice(), core.getQueue(), static_cast<UINT>(entityCount * 2));
		Scene scene(entityCount);

		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		LARGE_INTEGER before;
		QueryPerformanceCounter(&before);

#ifdef WINDOW_RENDERING
		while (winMngr.process()) {
#else
		for (int i = 0; i < 1000; ++i) {
#endif
			scene.update(core, bmpMngr, winMngr, rndrr);
		}

		LARGE_INTEGER after;
		QueryPerformanceCounter(&after);

		core.waitAll();

		std::cout
			<< entityCount
			<< " "
			<< (static_cast<double>(after.QuadPart - before.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart))
			<< " "
			<< scene.getHitCount()
			<< std::endl;
		
		// NOTE: 念のため、GPUを冷ますために2秒待つ。
		Sleep(2000);
	}
}
