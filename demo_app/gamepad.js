class GamePad {
    constructor(no) {
        this.no = no;
        this.prev_pressed = [];
        this.listener_list = { 'pressed': [], 'released': [] }

        this.updateGamepadStatus = this.updateGamepadStatus.bind(this);
        requestAnimationFrame(this.updateGamepadStatus);
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

    updateGamepadStatus() {
        const gamepads = navigator.getGamepads();
        const gamepad = gamepads[this.no];
        //console.log(gamepad);

        if (gamepad) {
            // ボタンの状態をチェック
            gamepad.buttons.forEach((button, index) => {
                if (button.pressed) {
                    if(!this.prev_pressed[index]) {
                         // 押されていない状態から押された状態になった場合にpressedイベントを発信する 
                        this.notify_event("pressed", { index: index });
                        this.prev_pressed[index] = true;
                    }
                } else {
                    if(this.prev_pressed[index]) {
                        // 押されていた状態から押されていない状態になった場合にreleasedイベントを発信する 
                        this.notify_event("released", { index: index });
                        this.prev_pressed[index] = false;
                    }
                }
            });
        }

        // 次のフレームでまたポーリングを実行
        requestAnimationFrame(this.updateGamepadStatus);
    }
}
