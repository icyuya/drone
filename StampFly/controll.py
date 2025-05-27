import serial
import time
import keyboard  # (キーボード入力用、別途 pip install keyboard が必要)

# ドローンが接続されているシリアルポートとボーレートを設定
SERIAL_PORT = '/dev/ttyACM1'  # Windows の場合。Linuxなら /dev/ttyUSBX, macOSなら /dev/cu.usbserial-XXXXX など
BAUD_RATE = 115200

# 操作値の初期化
yaw_val = 0.0
throttle_val = 0.0 # 0.0 (停止) ～ 1.0 (最大)
roll_val = 0.0   # -1.0 (左) ～ 1.0 (右)
pitch_val = 0.0  # -1.0 (奥) ～ 1.0 (手前)
arm_state = 0    # 0: Disarmed, 1: Armed
flip_state = 0   # 0: No flip, 1: Flip
flight_mode = 0  # 0: ANGLECONTROL, 1: RATECONTROL
alt_mode = 5     # 5: NOT_ALT_CONTROL_MODE, 4: ALT_CONTROL_MODE

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud.")
except serial.SerialException as e:
    print(f"Error opening serial port {SERIAL_PORT}: {e}")
    exit()

def send_command():
    # 例: Y:[yaw_val],T:[thr_val],R:[roll_val],P:[pitch_val],ARM:[arm_state],FLP:[flip_state],MD:[mode_val],AM:[alt_mode_val]\n
    command = f"Y:{yaw_val:.2f},T:{throttle_val:.2f},R:{roll_val:.2f},P:{pitch_val:.2f}," \
              f"ARM:{arm_state},FLP:{flip_state},MD:{flight_mode},AM:{alt_mode}\n"
    print(f"DEBUG: Sending to drone: {command.strip()}")
    try:
        ser.write(command.encode('ascii'))
        # print(f"Sent: {command.strip()}")
    except serial.SerialException as e:
        print(f"Error writing to serial port: {e}")
        # 必要に応じて再接続処理など

print("PC Control Started. Press 'q' to quit.")
print("Use w/s for throttle, a/d for roll, up/down for pitch, left/right for yaw.")
print("Space to arm/disarm, f for flip, m for flight mode, h for altitude mode.")

while True:
    # --- キーボード入力処理の例 (もっと洗練された方法があります) ---
    if keyboard.is_pressed('q'):
        print("DEBUG: Q pressed, quitting.")
        break
    
    # スロットル (W/S)
    if keyboard.is_pressed('w'):
        throttle_val = min(1.0, throttle_val + 0.05)
        print(f"DEBUG: W pressed, throttle_val: {throttle_val:.2f}")
    elif keyboard.is_pressed('s'):
        throttle_val = max(0.0, throttle_val - 0.05)
        print(f"DEBUG: S pressed, throttle_val: {throttle_val:.2f}")
    
    # ロール (A/D)
    if keyboard.is_pressed('d'):
        roll_val = min(1.0, roll_val + 0.05)
        print(f"DEBUG: D pressed, roll_val: {roll_val:.2f}")
    elif keyboard.is_pressed('a'):
        roll_val = max(-1.0, roll_val - 0.05)
        print(f"DEBUG: A pressed, roll_val: {roll_val:.2f}")
    # else: # 徐々に戻す
    #     roll_val *= 0.9
        
    # ピッチ (Up/Down Arrow)
    if keyboard.is_pressed('up'):
        pitch_val = min(1.0, pitch_val + 0.05)
        print(f"DEBUG: UP pressed, pitch_val: {pitch_val:.2f}")
    elif keyboard.is_pressed('down'):
        pitch_val = max(-1.0, pitch_val - 0.05)
        print(f"DEBUG: DOWN pressed, pitch_val: {pitch_val:.2f}")
    # else: # 徐々に戻す
    #     pitch_val *= 0.9

    # ヨー (Left/Right Arrow)
    if keyboard.is_pressed('right'):
        yaw_val = min(1.0, yaw_val + 0.05)
        print(f"DEBUG: RIGHT pressed, yaw_val: {yaw_val:.2f}")
    elif keyboard.is_pressed('left'):
        yaw_val = max(-1.0, yaw_val - 0.05)
        print(f"DEBUG: LEFT pressed, yaw_val: {yaw_val:.2f}")
    # else: # 徐々に戻す
    #     yaw_val *= 0.9

    # アーム (Space) - トグル
    if keyboard.is_pressed('space'):
        arm_state = 1 - arm_state # トグル
        print(f"DEBUG: SPACE pressed, arm_state: {arm_state}")
        time.sleep(0.2) # チャタリング防止

    # フリップ (F) - トグル
    if keyboard.is_pressed('f'):
        flip_state = 1 - flip_state
        print(f"DEBUG: F pressed, flip_state: {flip_state}")
        time.sleep(0.2)

    # フライトモード (M) - トグル
    if keyboard.is_pressed('m'):
        flight_mode = 1 - flight_mode # 0 と 1 をトグル
        print(f"DEBUG: M pressed, flight_mode: {flight_mode}")
        time.sleep(0.2)
    
    # 高度維持モード (H) - トグル
    if keyboard.is_pressed('h'):
        alt_mode = 4 if alt_mode == 5 else 5 # 4 と 5 をトグル
        print(f"DEBUG: H pressed, alt_mode: {alt_mode}")
        time.sleep(0.2)
    # --- ここまでキーボード入力処理の例 ---

    send_command()
    
    # ドローンからのテレメトリ受信 (もし実装する場合)
    # if ser.in_waiting > 0:
    #     telemetry_data = ser.readline().decode('ascii').strip()
    #     print(f"Drone: {telemetry_data}")

    time.sleep(0.02)  # 送信頻度 (例: 50Hz)

if ser.is_open:
    # 最後にスロットル0、ディスアームを送信
    throttle_val = 0.0
    arm_state = 0
    send_command()
    time.sleep(0.1)
    ser.close()
    print("Serial port closed.")