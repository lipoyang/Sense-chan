class GamePad {
    constructor(no) {
        // 十字ボタンのリピート周期[msec]
        this.repeatInterval = 100;
        // 十字ボタン
        this.directions = {
            up:    { button: 12, pressed: false, startTime: 0, lastTime: 0 },
            down:  { button: 13, pressed: false, startTime: 0, lastTime: 0 },
            left:  { button: 14, pressed: false, startTime: 0, lastTime: 0 },
            right: { button: 15, pressed: false, startTime: 0, lastTime: 0 }
        };
        // A, B, X, Y ボタン
        this.buttons = {
            A: { button: 0, pressed: false },
            B: { button: 1, pressed: false },
            X: { button: 2, pressed: false },
            Y: { button: 3, pressed: false }
        };

        this.no = no;
        this.listener_list = { 'pressed': [], 'released': [] }

        requestAnimationFrame((timestamp) => this.update(timestamp));
    }

    addEventListener(type, listener) {
        this.listener_list[type].push(listener);
    }

    notify_event(type, e) {
        let listeners = this.listener_list[type];
        for (let func of listeners) {
            func(e);
        }
    }

    update(timestamp) {
        const gamepads = navigator.getGamepads();
        const pad = gamepads[this.no];
        //console.log(gamepad);

        if (pad) {

            // --------------------
            // 十字キー
            // --------------------
            for (const [name, state] of Object.entries(this.directions)) {
                const pressed = pad.buttons[state.button].pressed;

                // 押した瞬間
                if (pressed && !state.pressed) {
                    this.notify_event("pressed", { index: state.button });
                    state.startTime = timestamp;
                    state.lastTime = timestamp;
                }

                // 押し続けている
                if (pressed && state.pressed) {
                    if (timestamp - state.lastTime >= this.repeatInterval)
                    {
                        this.notify_event("pressed", { index: state.button });
                        state.lastTime = timestamp;
                    }
                }
                // 離した瞬間
                if (!pressed && state.pressed) {
                    this.notify_event("released", { index: state.button });
                }

                state.pressed = pressed;
            }

            // --------------------
            // A / B / X / Y
            // --------------------
            for (const [name, state] of Object.entries(this.buttons)) {
                const pressed = pad.buttons[state.button].pressed;

                // 押した瞬間だけ
                if (pressed && !state.pressed) {
                    this.notify_event("pressed", { index: state.button });
                }

                state.pressed = pressed;
            }
        }
        // 次のフレームでまたポーリングを実行
        requestAnimationFrame((timestamp) => this.update(timestamp));
    }
}
