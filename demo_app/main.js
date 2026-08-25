/********** 共通定数/変数 ***********/

// モード
const OpMode = {
  ModeUrochoro    : 0,
  ModeModeRadicon : 1,
  ModeGyro        : 2,
  ModeOffline     : 3
};
let opMode = OpMode.ModeOffline;

// ステータス項目
const StatusItem = {
  StatusComm    : 0,
  StatusBattery : 1,
  StatusStatus  : 2
};

// 接続状態
const BLEStatus = {
  Disconnected : 0,
  Connecting   : 1,
  Connected    : 2
};
let btState = BLEStatus.Disconnected;

// 送信コマンドの番号
const CH = {
  ST:0,
  TH:1,
  MODE: 2,
  ALL:3
};

/********** 画面表示 ***********/

const MsgOffline    = ["ｾﾝｽﾁｬﾝとつながっていません。", "「つなげる」ボタンを押してください。"];
const MsgConnecting = ["ｽﾀｯｸﾁｬﾝに接続中です…"];
const MsgUrochoro   = ["うろちょろモードで動作中です。", "気まぐれにうろちょろしています。"];
const MsgRadicon    = ["ラジコンモードで動作中です。", "キーボードの矢印キーで操縦できます。"];
const MsgGyro       = ["ジャイロモードで動作中です。", "床が回転しても同じ方向を向き続けます。"];
const ModeMessages  = [MsgUrochoro, MsgRadicon, MsgGyro, MsgOffline];

// 座標
const ButtonX = 60;
const ButtonY = [150, 350, 550];
const ButtonW = 500;
const ButtonH = 140;
const ConnectX = 710;
const ConnectY = 650;
const ConnectW = 500;
const ConnectH = 140;

// 色
const bg_color = "#c5cae9";
const fg_color = "#3949ab";
const window_color  = "#e8eaf6";
const cautoin_color = "#ff1744";

// キャンバス
const canvas = document.getElementById('myCanvas');
const context = canvas.getContext('2d');

// 画像
const background   = new Image();
const buttonNormal = new Image();
const buttonActive = new Image();
const buttonLowKey = new Image();

// 画面表示の縮尺
let scale = 1;

// 画像データ読み込み
async function loadImages() {
  
  background.src   = "background.png";    // 背景画像
  buttonNormal.src = "button_normal.png"; // 通常ボタン画像
  buttonActive.src = "button_active.png"; // アクティブボタン画像
  buttonLowKey.src = "button_lowkey.png"; // 目立たないボタン画像

  // decode() は画像のデコード完了を保証する（onload より確実）
  await Promise.all([
    background.decode(),
    buttonNormal.decode(),
    buttonActive.decode(),
    buttonLowKey.decode(),
    document.fonts.load("72px 'GenEiMGothic2'")
  ]);
}

// 画像データを読み込んで表示
loadImages().then(() => {
  resizeAndDraw();
  window.addEventListener("resize", () => resizeAndDraw());
});

// 画面サイズに合わせてcanvasをリサイズして描画
function resizeAndDraw()
{
  const screenW = window.innerWidth;
  const screenH = window.innerHeight;
  const imgW = background.width;
  const imgH = background.height;

  scale = Math.min(screenW / imgW, screenH / imgH);
  const drawW = imgW * scale;
  const drawH = imgH * scale;

  canvas.width = drawW;
  canvas.height = drawH;
  
  // 背景描画
  context.clearRect(0, 0, drawW, drawH);
  context.drawImage(background, 0, 0, drawW, drawH);

  // ステータスのラベル表示
  drawStatusLabel();
  // モードボタン描画
  drawModeButton(opMode);
  // 接続ボタン描画
  drawConnectButton(btState);

  // ステータス用の文字
  if(btState == BLEStatus.Connected){
    drawStatusValue(StatusItem.StatusComm, "Online", false);
  }else{
    drawStatusValue(StatusItem.StatusComm, "Offline", true);
  }
  drawStatusValue(StatusItem.StatusBattery, "---", false);
  drawStatusValue(StatusItem.StatusStatus, "---", false);

  // メッセージの表示
  drawMessage(ModeMessages[opMode]);
}

// モードボタン描画
function drawModeButton(mode)
{
  const ButtonText = ["うろちょろ", "ラジコン", "ジャイロ"];

  context.font = `${72 * scale}px "GenEiMGothic2"`;
  context.textAlign = "center";
  context.textBaseline = "middle";

  for(let i=0; i<3; i++)
  {
    const bx = ButtonX * scale;
    const by = ButtonY[i] * scale;
    const bw = ButtonW * scale;
    const bh = ButtonH * scale;

    context.fillStyle = bg_color;
    context.fillRect(bx-1, by-1, bw+2, bh+2);

    context.fillStyle = "white";
    if(i == mode && btState == BLEStatus.Connected){
      context.drawImage(buttonActive, bx, by, bw, bh);
    }else{
      context.drawImage(buttonNormal, bx, by, bw, bh);
    }
    const tx = bx + bw / 2;
    const ty = by + bh / 2;
    context.fillText(ButtonText[i], tx, ty);
  }
}

// 接続ボタン描画
function drawConnectButton(state)
{
  const connectText   = (state == BLEStatus.Disconnected) ? "つなげる" : "おわる";
  const connectButton = (state == BLEStatus.Disconnected) ? buttonNormal : buttonLowKey;

  const bx = ConnectX * scale;
  const by = ConnectY * scale;
  const bw = ConnectW * scale;
  const bh = ConnectH * scale;
  const tx = bx + bw / 2;
  const ty = by + bh / 2;

  context.fillStyle = bg_color;
  context.fillRect(bx-1, by-1, bw+2, bh+2);
  context.drawImage(connectButton, bx, by, bw, bh);

  context.fillStyle = "white";
  context.font = `${72 * scale}px "GenEiMGothic2"`;
  context.textAlign = "center";
  context.textBaseline = "middle";
  context.fillText(connectText, tx, ty);
}

// ステータスのラベル描画
function drawStatusLabel()
{
  const LabelX = 1425;
  const LabelY = [200, 380, 560];
  const LabelText = ["Comm:", "Battery:", "Status:"];

  context.fillStyle = fg_color;
  context.font = `${56 * scale}px "GenEiMGothic2"`;
  context.textAlign = "left";
  context.textBaseline = "top";

  for(let i=0; i<3; i++)
  {
    const lx = LabelX * scale;
    const ly = LabelY[i] * scale;
    context.fillText(LabelText[i], lx, ly);
  }
}

// ステータスの値描画
function drawStatusValue(index, value, caution)
{
  const ValueX = 1500;
  const ValueY = [290, 470, 650];
  const ValueW = 320;
  const ValueH = 70;

  const vx = ValueX * scale;
  const vy = ValueY[index] * scale;
  const vw = ValueW * scale;
  const vh = ValueH * scale;
  
  const color = (caution) ? cautoin_color : fg_color;

  context.fillStyle = window_color;
  context.fillRect(vx-1, vy-1, vw+2, vh+2);

  context.fillStyle = color;
  context.font = `${56 * scale}px "GenEiMGothic2"`;
  context.textAlign = "left";
  context.textBaseline = "top";
  context.fillText(value, vx, vy);
}

// メッセージ描画
function drawMessage(message)
{
  const TextX = 130;
  const TextY = 950;
  const TextL = 120;
  const TextW = 1640;
  const TextH = 240;

  const tx = TextX * scale;
  const ty = TextY * scale;
  const tl = TextL * scale;
  const tw = TextW * scale;
  const th = TextH * scale; 

  context.fillStyle = window_color;
  context.fillRect(tx-1, ty-1, tw+2, th+2);

  context.fillStyle = fg_color;
  context.font = `${72 * scale}px "GenEiMGothic2"`;
  context.textAlign = "left";
  context.textBaseline = "top";

  message.forEach((text, i) => {
    context.fillText(text, tx, ty + i * tl);
  });
}

/********** マウス/タッチイベント ***********/
canvas.addEventListener("mousedown", async function (e) {

  const rect = canvas.getBoundingClientRect();

  const x = event.clientX - rect.left;
  const y = event.clientY - rect.top;

  for(let i = 0; i < 3; i++){
    const bx1 = ButtonX * scale;
    const bx2 = (ButtonX + ButtonW) * scale;
    const by1 = ButtonY[i] * scale;
    const by2 = (ButtonY[i] + ButtonH) * scale;
    if(x > bx1 && x < bx2 && y > by1 && y < by2){
        onClickModeButton(i);
    }
  }
  {
    const bx1 = ConnectX * scale;
    const bx2 = (ConnectX + ConnectW) * scale;
    const by1 = ConnectY * scale;
    const by2 = (ConnectY + ConnectH) * scale;
    if(x > bx1 && x < bx2 && y > by1 && y < by2){
        onClickConnectButton();
    }
  }
}); 

// モードボタン
async function onClickModeButton(mode)
{
    console.log("onClickModeButton:" + mode);

    if(btState != BLEStatus.Connected) return;

    opMode = mode;
    drawModeButton(opMode);
    drawMessage(ModeMessages[opMode]);

    const command = "#M" + ("0" + mode.toString(16).toUpperCase()).slice(-2) + "$\r\n";
    sendBLE(CH.MODE, command);
}

// 接続/切断ボタン
async function onClickConnectButton()
{
    console.log("onClickConnectButton:");

    if(btState != BLEStatus.Connected){
      await connectBLE();
    }else{
      disconnectBLE();
    }
 }

/********** キーボードイベント ***********/
document.addEventListener("keydown", async (event) => {
    switch (event.key) {
        // 矢印キー
        case "ArrowUp":
            console.log("TH 前");
            sendTH(1.0);
            break;
        case "ArrowDown":
            console.log("TH 後");
            sendTH(-1.0);
            break;
        case "ArrowLeft":
            console.log("ST 左");
            sendST(1.0);
            break;
        case "ArrowRight":
            console.log("ST 右");
            sendST(-1.0);
            break;
        // C: 接続/切断
        case "c":
            console.log("C");
            if(!event.repeat) {
              onClickConnectButton();
            }
            break;
        // U: うろちょろ
        case "u":
            console.log("U");
            if(!event.repeat) {
              onClickModeButton(OpMode.ModeUrochoro);
            }
            break;
        // R: ラジコン
        case "r":
            console.log("R");
            if(!event.repeat) {
              onClickModeButton(OpMode.ModeModeRadicon);
            }
            break;
        // G: ジャイロ
        case "g":
            console.log("G");
            if(!event.repeat) {
              onClickModeButton(OpMode.ModeGyro);
            }
            break;
    }
});
document.addEventListener("keyup", async (event) => {
    switch (event.key) {
        // 矢印キー
        case "ArrowUp":
        case "ArrowDown":
            console.log("TH 停止");
            sendTH(0.0);
            break;
        case "ArrowLeft":
        case "ArrowRight":
            console.log("ST 停止");
            sendST(0.0);
            break;
    }
});

/********** ゲームパッドイベント ***********/
const gamepad = new GamePad();

gamepad.addEventListener("pressed", btnDownHandler);
gamepad.addEventListener("released", btnUpHandler);

// ボタン押下イベント
function btnDownHandler(event) {
  //console.log("gamepad down:" + event.index);
  switch(event.button) {
    case "A":
      console.log("R");
      if(!event.repeat) {
        onClickModeButton(OpMode.ModeModeRadicon);
      }
      break;
    case "B":
      console.log("G");
      if(!event.repeat) {
        onClickModeButton(OpMode.ModeGyro);
      }
      break;
    case "X":
      console.log("U");
      if(!event.repeat) {
        onClickModeButton(OpMode.ModeUrochoro);
      }
      break;
    case "Y":
      break;
    case "up":
      console.log("TH 前");
      sendTH(1.0);
      break;
    case "down":
      console.log("TH 後");
      sendTH(-1.0);
      break;
    case "left":
      console.log("ST 左");
      sendST(1.0);
      break;
    case "right":
      console.log("ST 右");
      sendST(-1.0);
      break;
  }
}

// ボタン解放イベント
function btnUpHandler(event) {
  //console.log("gamepad up:" + event.index);
  switch(event.button) {
    case "up":
    case "down":
      console.log("TH 停止");
      sendTH(0.0);
      break;
    case "left":
    case "right":
      console.log("ST 停止");
      sendST(0.0);
      break;
    }
}

// ステアリングの送信
// st = -1.0 ... +1.0
async function sendST(st)
{
  // console.log("ST = " +st);

  if(btState != BLEStatus.Connected) return;

  let bST = Math.floor(st * 127);
  if(bST<0) bST += 256;
  const command = "#T" + ("0" + bST.toString(16).toUpperCase()).slice(-2) + "$\r\n";
  sendBLE(CH.ST, command);
}

// スロットルの送信
// th = -1.0 ... +1.0
async function sendTH(th)
{
  // console.log("TH = " +th);

  if(btState != BLEStatus.Connected) return;

  let bTH = Math.floor(th * 127);
  if(bTH<0) bTH += 256;
  const command = "#D" + ("0" + bTH.toString(16).toUpperCase()).slice(-2) + "$\r\n";
  sendBLE(CH.TH, command);
}

/********** BLE関連 ***********/

// BLEサービスのUUID
const UUID_NUS_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const UUID_NUS_RX_CHAR = '6e400003-b5a3-f393-e0a9-e50e24dcca9e';
const UUID_NUS_TX_CHAR = '6e400002-b5a3-f393-e0a9-e50e24dcca9e';

// BLEデバイス
let bleDevice = null;
// BLEキャラクタリスティック
let chrRX;
let chrTX;

// 定期送信用タイマー
let keepAliveInterval = null;

// 前回送信時刻
let last_time = 0;
// 送信保留タイマ
let pending_timer = null;
// 送信保留中のコマンド
let pendingCommand = Array(CH.ALL).fill("");
// 送信インターバル[msec]
const SEND_INTERVAL = 50;

// BLE接続
async function connectBLE() {
  try {
    btState = BLEStatus.Connecting;
    opMode = OpMode.ModeOffline;
    drawModeButton(opMode);
    drawConnectButton(btState);
    drawStatusValue(StatusItem.StatusComm, "Trying", true);
    drawStatusValue(StatusItem.StatusBattery, "---", false);
    drawStatusValue(StatusItem.StatusStatus, "---", false);
    drawMessage(MsgConnecting);

    // デバイスを取得 (サービスのUUIDでフィルタ)
    console.log("Requesting Bluetooth Device...");
    bleDevice = await navigator.bluetooth.requestDevice({
        filters: [{ services: [UUID_NUS_SERVICE] }],
    });
    // 切断時イベントハンドラの登録
    bleDevice.addEventListener('gattserverdisconnected', onDisconnected);
    // デバイスに接続
    console.log("Connecting to GATT Server...");
    const server = await bleDevice.gatt.connect();
    // サービスを取得
    console.log("Getting Service...");
    const service = await server.getPrimaryService(UUID_NUS_SERVICE);
    // キャラクタリスティックを取得
    console.log("Getting Characteristics...");
    chrRX         = await service.getCharacteristic(UUID_NUS_RX_CHAR);
    chrTX         = await service.getCharacteristic(UUID_NUS_TX_CHAR);
    // 受信時の処理
    chrRX.addEventListener('characteristicvaluechanged', onReceived);
    chrRX.startNotifications();

    btState = BLEStatus.Connected;
    opMode = OpMode.ModeUrochoro;
    drawModeButton(opMode);
    drawConnectButton(btState);
    drawStatusValue(StatusItem.StatusComm, "Online", false);
    drawStatusValue(StatusItem.StatusBattery, "---", false);
    drawStatusValue(StatusItem.StatusStatus, "---", false);
    drawMessage(MsgUrochoro);

    // 定期送信タイマー開始
    if (!keepAliveInterval) {
      keepAliveInterval = setInterval(async () => {
        if (chrTX) {
          await sendBLE_NoRetry("#K$\r\n");
        }
      }, 1000); // 1秒ごと
    }

  } catch (error) {
    console.log("ERROR! " + error);
    bleDevice = null;

    btState = BLEStatus.Disconnected;
    opMode = OpMode.ModeOffline;
    drawModeButton(opMode);
    drawConnectButton(btState);
    drawStatusValue(StatusItem.StatusComm, "Offline", true);
    drawStatusValue(StatusItem.StatusBattery, "---", false);
    drawStatusValue(StatusItem.StatusStatus, "---", false);
    drawMessage(MsgOffline);
  }
}

// BLE切断
function disconnectBLE(){
  if(bleDevice != null){
    bleDevice.gatt.disconnect();
  }
}

// BLE送信サブルーチン (実際に送信する処理)
async function sendBLE_Sub(command)
{
  last_time = performance.now();
  const encoder = new TextEncoder();
  const byteArray = encoder.encode(command);
  await chrTX.writeValueWithoutResponse(byteArray).then(() => {
    // console.log('\u001b[32m' + `send: ${command}` + '\u001b[0m');
  }).catch(()=>{
    // console.log('\u001b[31m' + `send ERROR: ${command}` + '\u001b[0m');
  });
}

// BLE送信リトライ処理
async function sendBLE_Retry()
{
  pending_timer = null;
  let command = "";
  for(let i = 0; i < CH.ALL; i++){
    command += pendingCommand[i];
    pendingCommand[i] = "";
  }
  sendBLE_Sub(command);
}

// BLE送信
async function sendBLE(ch, command)
{
  // 既に保留中なら送信コマンドのみ更新して今は送信しない
  if(pending_timer != null){
    pendingCommand[ch] = command; // 送信保留するコマンド
    return;
  }
  // 最後の送信から送信インターバル未満なら送信を保留する
  // (送信インターバル経過後の送信を予約する)
  const now = performance.now();
  const elapsed = now - last_time;
  if(elapsed < SEND_INTERVAL){
    pendingCommand[ch] = command; // 送信保留するコマンド
    const delay = SEND_INTERVAL - elapsed; // 送信まで待つ時間
    pending_timer = setTimeout( sendBLE_Retry, delay);
    return;
  }
  // 送信インターバル経過後であれば即送信する
  else{
    sendBLE_Sub(command);
  }
}

// BLE送信 (送信リトライなし)
async function sendBLE_NoRetry(command)
{
  // 既に保留中なら送信しない
  if(pending_timer != null){
    return;
  }
  // 最後の送信から送信インターバル未満なら送信しない
  const now = performance.now();
  const elapsed = now - last_time;
  if(elapsed < SEND_INTERVAL){
    return;
  }
  // 送信インターバル経過後であれば即送信する
  else{
    sendBLE_Sub(command);
  }
}

// 切断時
function onDisconnected(event) {
  const device = event.target;
  console.log(`Device ${device.name} is disconnected.`);
  bleDevice = null;

  // 定期送信タイマー停止
  if (keepAliveInterval) {
    clearInterval(keepAliveInterval);
    keepAliveInterval = null;
  }

  btState = BLEStatus.Disconnected;
  opMode = OpMode.ModeOffline;
  drawModeButton(opMode);
  drawConnectButton(btState);
  drawStatusValue(StatusItem.StatusComm, "Offline", true);
  drawStatusValue(StatusItem.StatusBattery, "---", false);
  drawStatusValue(StatusItem.StatusStatus, "---", false);
  drawMessage(MsgOffline);
}

// 受信時
function onReceived(event) {
  const characteristic = event.target;
  const value = characteristic.value;
  const decoder = new TextDecoder();
  const text = decoder.decode(value);
  
  // console.log(text); // 受信した文字列

  if(text[0] == "#"){
    if(text[1] == "B"){
      const voltage = parseInt(text.substring(2, 6), 16) / 1000;
      const voltage_str = voltage.toFixed(2) + " V";
      const LOW_BATTERY = 3.55;
      const isLowBatt = (voltage < LOW_BATTERY);
      const status = parseInt(text.substring(6, 10), 16) / 1000;
      let status_str = "";
      let isError = false;
      if(status == 0){
        status_str = "Normal";
      }else{
        isError = true;
        if(status == 0x0001){
          status_str = "Gyro Err"
        }else{
          status_str = "Abnormal"
        }
      }
      drawStatusValue(StatusItem.StatusBattery, voltage_str, isLowBatt);
      drawStatusValue(StatusItem.StatusStatus, status_str, isError);
    }else{
      console.log("unknown command");
    }
  }else{
      console.log("invalid message");
  }
}
