// ゲームパッド
class GamePad {
    // コンストラクタ
    constructor() {
        // 使用中のゲームパッド
        this.pad = null;
        // 十字ボタンのリピート周期[msec]
        this.repeatInterval = 100;
        // 十字ボタン
        this.directions = {
            up:    { pressed: false, lastTime: 0 },
            down:  { pressed: false, lastTime: 0 },
            left:  { pressed: false, lastTime: 0 },
            right: { pressed: false, lastTime: 0 }
        };
        // A, B, X, Y ボタン
        this.buttons = {
            A: { pressed: false },
            B: { pressed: false },
            X: { pressed: false },
            Y: { pressed: false }
        };

        this.listener_list = { 'pressed': [], 'released': [] }

        // 更新処理を開始
        requestAnimationFrame((timestamp) => this.update(timestamp));
    }

    // イベントリスナーを追加
    addEventListener(type, listener) {
        this.listener_list[type].push(listener);
    }

    // イベントを通知
    notify_event(type, e) {
        let listeners = this.listener_list[type];
        for (let func of listeners) {
            func(e);
        }
    }

    // 更新処理
    update(timestamp) {
        // 最初に見つかったゲームパッドを使用
        let pad = null;
        const gamepads = navigator.getGamepads();
        for(let p of gamepads){
            if(p != null && p.buttons.length > 4){ // ボタン数4の仮想HIDゲームパッドを除外
                if (p.connected) pad = p;
            }                
        }
        if(this.pad != null && pad == null){
            console.log("Gamepad disconnected!");
        }
        if((this.pad == null && pad != null) ||
           (this.pad != null && pad != null && this.pad.id !== pad.id)){
            console.log(`Gamepad = ${pad.id}`);
        }
        this.pad = pad;

        // 十字キーとボタンの判定
        if (pad) {
            this.updateDirections(timestamp);
            this.updateButtons();
        }
        // 次のフレームでまたポーリングを実行
        requestAnimationFrame((timestamp) => this.update(timestamp));
    }

    // 十字キーの判定
    updateDirections(timestamp) {
        const directions = {
            up:    this.isDirectionPressed("up"),
            down:  this.isDirectionPressed("down"),
            left:  this.isDirectionPressed("left"),
            right: this.isDirectionPressed("right")
        };

        for (const [name, pressed] of Object.entries(directions)) {
            const state = this.directions[name];

            // 押した瞬間
            if (pressed && !state.pressed) {
                this.notify_event("pressed", { button: name });
                state.lastTime = timestamp;
            }
            // 押し続けている
            if (pressed && state.pressed) {
                if (timestamp - state.lastTime >= this.repeatInterval)
                {
                    this.notify_event("pressed", { button: name });
                    state.lastTime = timestamp;
                }
            }
            // 離した瞬間
            if (!pressed && state.pressed) {
                this.notify_event("released", { button: name });
            }

            state.pressed = pressed;
        }
    }

    // 十字キーが押されているか？
    isDirectionPressed(direction) {
        const pad = this.pad;

        // Standard Gamepad形式
        if (pad.mapping === "standard" && pad.buttons.length > 15)
        {
            const buttonNumbers = {
                up: 12,
                down: 13,
                left: 14,
                right: 15
            };
            return pad.buttons[buttonNumbers[direction]].pressed;
        }
        // AXIS形式
        if (pad.axes.length >= 2) {
            const x = pad.axes[0];
            const y = pad.axes[1];

            switch (direction) {
                case "up":
                    return y < -0.5;

                case "down":
                    return y > 0.5;

                case "left":
                    return x < -0.5;

                case "right":
                    return x > 0.5;
            }
        }
        return false;
    }

    // A/B/X/Yボタン判定
    updateButtons() {
        const buttonNumbers = {
            A: 0,
            B: 1,
            X: (this.pad.mapping === "standard") ? 2 : 3,
            Y: (this.pad.mapping === "standard") ? 3 : 4
        };
        for (const [name, state] of Object.entries(this.buttons)) {
            const button = this.pad.buttons[buttonNumbers[name]];
            if (!button) continue;

            const pressed = button.pressed;

            // 押した瞬間だけ
            if (pressed && !state.pressed) {
                this.notify_event("pressed", { button: name });
            }
            state.pressed = pressed;
        }
    }
}
