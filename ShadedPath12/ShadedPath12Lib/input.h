class Camera;
// count keyboard events, will be cleared after next call to getAndClearKeyTicks()
struct KeyTicks {
	int tickUp = 0;
	int tickDown = 0;
	int tickLeft = 0;
	int tickRight = 0;
	void clear() {
		tickDown = 0;
		tickLeft = 0;
		tickRight = 0;
		tickUp = 0;
	};
};

// singleton class to receive all input from keyboard/mouse
class Input {
public:
	static Input* getInstance() {
		static Input input;
		return &input;
	};
	BYTE key_state[256];
	int mouseDx;
	int mouseDy;
	bool mouseTodo;
	void updateKeyboardState();
	bool keyDown(BYTE key);
	bool anyKeyDown = false;
	void getAndClearKeyTicks(KeyTicks& keyTicks);
	void applyTicksToCameraPosition(KeyTicks& keyTicks, Camera* c, float dt);
	void applyMouseEvents(Camera* c, float dt);
private:
	Input() {};							// prevent creation outside this class
	Input(const Input&);				// prevent creation via copy-constructor
	Input& operator = (const Input&);	// prevent instance copies
	KeyTicks kticks;
};