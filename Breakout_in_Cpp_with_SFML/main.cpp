#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

const unsigned short window_width = 600;
const unsigned short window_height = 600;

using namespace std;
using namespace sf;

struct GameContext {
	unique_ptr<RenderWindow> m_window;

	GameContext() {
		m_window = make_unique<RenderWindow>();
	}
};

class Ball {
private:
	shared_ptr<GameContext> context;
public:
	float x, y, radius, speedX, speedY;

	Ball(shared_ptr<GameContext> context_in) : context(context_in) {
		x = window_width / 2.0f;
		y = window_height / 2.0f + 100.0f;
		radius = 10.0f;
		speedX = 300.0f;
		speedY = -300.0f;
	}

	void Update(float dt) {
		x += speedX * dt;
		y += speedY * dt;

		if (x - radius <= 0 || x + radius >= window_width) {
			speedX = -speedX;
		}

		if (y - radius <= 0) {
			speedY = -speedY;
		}
	}

	void Draw() {
		CircleShape ball((float)radius);
		ball.setFillColor(Color::White);
		ball.setOrigin(radius, radius);
		ball.setPosition(x, y);

		context->m_window->draw(ball);
	}
};

class Paket {
private:
	shared_ptr<GameContext> context;
public:
	float x, y, width, height, speed;

	Paket(shared_ptr<GameContext> context_in) : context(context_in) {
		width = 100.0f;
		height = 10.0f;
		x = window_width / 2 - width / 2;
		y = window_height - height - 15.0f;
		speed = 500.0f;
	}

	void Update(float dt) {
		if (Keyboard::isKeyPressed(Keyboard::A) || Keyboard::isKeyPressed(Keyboard::Left)) {
			x -= speed * dt;
		}
		else if (Keyboard::isKeyPressed(Keyboard::D) || Keyboard::isKeyPressed(Keyboard::Right)) {
			x += speed * dt;
		}

		if (x <= 0) x = 0;
		if (x + width >= window_width) x = window_width - width;
	}

	void Draw() {
		RectangleShape paket(Vector2f((float)width, (float)height));
		paket.setFillColor(Color::White);
		paket.setPosition(x, y);

		context->m_window->draw(paket);
	}
};

struct Block_struct {
	RectangleShape shape;
	bool isDestroyed = false;
	short hp = 1;
};

class BlockManager {
private:
	shared_ptr<GameContext> context;
public:
	short rows = 3;
	short cols = 5;
	float start_x = 37.0f;
	float start_y = 40.0f;
	float block_width = 100.0f;
	float block_height = 20.0f;
	float padding = 10.0f;

	vector<Block_struct> blocks;

	BlockManager(shared_ptr<GameContext> context_in) : context(context_in) {
		ResetBlocks();
	}

	void ResetBlocks() {
		blocks.clear();
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++) {
				Block_struct b;
				b.shape.setSize(Vector2f(block_width, block_height));

				Uint8 r = rand() % 206 + 10;
				Uint8 g = rand() % 206 + 10;
				Uint8 b_color = rand() % 206 + 10;

				b.shape.setFillColor(Color(r, g, b_color));

				float bx = start_x + j * (block_width + padding);
				float by = start_y + i * (block_height + padding);
				b.shape.setPosition(bx, by);

				blocks.push_back(b);
			}
		}
	}

	void Draw() {
		for (auto& b : blocks) {
			if (!b.isDestroyed) {
				context->m_window->draw(b.shape);
			}
		}
	}
};

class Game {
private:
	shared_ptr<GameContext> m_context;
	Ball ball;
	Paket paket;
	BlockManager blockManager;
	bool game_over;
	bool pause;
	bool win;

	Font font;
	Text pause_text;
	Text game_over_text;
	Text win_text;

public:
	Game(shared_ptr<GameContext> context, Ball ball_in, Paket paket_in, BlockManager block_in)
		: m_context(context), ball(ball_in), paket(paket_in), blockManager(block_in)
	{
		game_over = false;
		pause = false;
		win = false;

		font.loadFromFile("Font/times.ttf");

		pause_text.setFont(font);
		pause_text.setString("PAUSE. Press P to resume");
		pause_text.setCharacterSize(30);
		pause_text.setFillColor(Color::White);
		pause_text.setPosition(window_width / 2 - 150, window_height / 2 - 30);

		game_over_text.setFont(font);
		game_over_text.setString("GAME OVER. Press R to restart");
		game_over_text.setCharacterSize(30);
		game_over_text.setFillColor(Color::Red);
		game_over_text.setPosition(window_width / 2 - 180, window_height / 2 - 30);

		win_text.setFont(font);
		win_text.setString("WIN! Press R to restart");
		win_text.setCharacterSize(30);
		win_text.setFillColor(Color::Green);
		win_text.setPosition(window_width / 2 - 150, window_height / 2 - 30);
	}

	void TogglePause() {
		if (!game_over && !win) pause = !pause;
	}

	void Update(float dt) {
		// Якщо пауза, програш або перемога — чекаємо перезапуску через Event в main
		if (pause || game_over || win) {
			return;
		}

		ball.Update(dt);
		paket.Update(dt);
		CollisionBallWithPaket();
		CollisionBallWithBlocks();
		GameOver();
		Win();
	}

	void CollisionBallWithPaket() {
		if (ball.y + ball.radius >= paket.y &&
			ball.y - ball.radius <= paket.y + paket.height &&
			ball.x >= paket.x &&
			ball.x <= paket.x + paket.width) {

			ball.speedY = -abs(ball.speedY);
			ball.y = paket.y - ball.radius;
		}
	}

	void CollisionBallWithBlocks() {
		for (auto& b : blockManager.blocks) {
			if (b.isDestroyed) continue;

			Vector2f pos = b.shape.getPosition();
			float bw = blockManager.block_width;
			float bh = blockManager.block_height;

			if (ball.x + ball.radius >= pos.x &&
				ball.x - ball.radius <= pos.x + bw &&
				ball.y + ball.radius >= pos.y &&
				ball.y - ball.radius <= pos.y + bh) {

				b.isDestroyed = true;
				ball.speedY = -ball.speedY;
				break;
			}
		}
	}

	void GameOver() {
		if (ball.y - ball.radius > window_height) {
			game_over = true;
		}
	}

	void Win() {
		bool all_destroyed = true;
		for (const auto& b : blockManager.blocks) {
			if (!b.isDestroyed) {
				all_destroyed = false;
				break;
			}
		}

		if (all_destroyed) {
			win = true;
		}
	}

	void Reset() {
		ball = Ball(m_context);
		paket = Paket(m_context);
		blockManager.ResetBlocks();
		game_over = false;
		pause = false;
		win = false;
	}

	void Draw() {
		ball.Draw();
		paket.Draw();
		blockManager.Draw();

		if (pause) m_context->m_window->draw(pause_text);
		if (game_over) m_context->m_window->draw(game_over_text);
		if (win) m_context->m_window->draw(win_text);
	}
};

int main() {
	srand(static_cast<unsigned int>(time(NULL)));
	shared_ptr<GameContext> m_context = make_shared<GameContext>();
	m_context->m_window->create(VideoMode(window_width, window_height), "Breakout");

	Ball ball(m_context);
	Paket paket(m_context);
	BlockManager blockManager(m_context);
	Game game(m_context, ball, paket, blockManager);

	Clock clock;

	while (m_context->m_window->isOpen()) {
		Event event;
		while (m_context->m_window->pollEvent(event)) {
			if (event.type == Event::Closed)
				m_context->m_window->close();

			if (event.type == Event::KeyPressed) {
				if (event.key.code == Keyboard::P) {
					game.TogglePause();
				}
				if (event.key.code == Keyboard::R) {
					game.Reset();
				}
			}
		}

		float dt = clock.restart().asSeconds();

		game.Update(dt);

		m_context->m_window->clear();
		game.Draw();
		m_context->m_window->display();
	}

	return 0;
}