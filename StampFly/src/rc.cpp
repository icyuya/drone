#include "rc.hpp"
#include "flight_control.hpp"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// esp_now_peer_info_t slave;

volatile uint16_t Connect_flag = 0;

// Telemetry相手のMAC ADDRESS 4C:75:25:AD:B6:6C
// ATOM Lite (C): 4C:75:25:AE:27:FC
// 4C:75:25:AD:8B:20
// 4C:75:25:AF:4E:84
// 4C:75:25:AD:8B:20
// 4C:75:25:AD:8B:20 赤水玉テープ　ATOM lite
uint8_t TelemAddr[6] = {0x4C, 0x75, 0x25, 0xAD, 0x8B, 0x20};
// uint8_t TelemAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
volatile uint8_t MyMacAddr[6];
volatile uint8_t Rc_err_flag = 0;
esp_now_peer_info_t peerInfo;

// RC
volatile float Stick[16];
volatile uint8_t Recv_MAC[3];

// 受信コールバック
// void OnDataRecv(const uint8_t *mac_addr, const uint8_t *recv_data, int data_len)
// {
//   Connect_flag=0;

//   uint8_t* d_int;
//   //int16_t d_short;
//   float d_float;

//   Recv_MAC[0]=recv_data[0];
//   Recv_MAC[1]=recv_data[1];
//   Recv_MAC[2]=recv_data[2];

//   if ((recv_data[0]==MyMacAddr[3])&&(recv_data[1]==MyMacAddr[4])&&(recv_data[2]==MyMacAddr[5]))
//   {
//     Rc_err_flag = 0;
//   }
//   else
//   {
//     Rc_err_flag = 1;
//     return;
//   }

//   d_int = (uint8_t*)&d_float;
//   d_int[0] = recv_data[3];
//   d_int[1] = recv_data[4];
//   d_int[2] = recv_data[5];
//   d_int[3] = recv_data[6];
//   Stick[RUDDER]=d_float;

//   d_int[0] = recv_data[7];
//   d_int[1] = recv_data[8];
//   d_int[2] = recv_data[9];
//   d_int[3] = recv_data[10];
//   Stick[THROTTLE]=d_float;

//   d_int[0] = recv_data[11];
//   d_int[1] = recv_data[12];
//   d_int[2] = recv_data[13];
//   d_int[3] = recv_data[14];
//   Stick[AILERON]  = d_float;

//   d_int[0] = recv_data[15];
//   d_int[1] = recv_data[16];
//   d_int[2] = recv_data[17];
//   d_int[3] = recv_data[18];
//   Stick[ELEVATOR]  = d_float;

//   Stick[BUTTON_ARM] = recv_data[19];
//   Stick[BUTTON_FLIP] = recv_data[20];
//   Stick[CONTROLMODE] = recv_data[21];
//   Stick[ALTCONTROLMODE] = recv_data[22];

//   Stick[LOG] = 0.0;

// #if 0
//   USBSerial.printf("%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f %6.3f  %6.3f\n\r",
//                                             Stick[THROTTLE],
//                                             Stick[AILERON],
//                                             Stick[ELEVATOR],
//                                             Stick[RUDDER],
//                                             Stick[BUTTON_ARM],
//                                             Stick[BUTTON_FLIP],
//                                             Stick[CONTROLMODE],
//                                             Stick[ALTCONTROLMODE],
//                                             Stick[LOG]);
// #endif
// }

// 送信コールバック
// uint8_t esp_now_send_status;
// void on_esp_now_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//   esp_now_send_status = status;
// }

// --- PCからのシリアルコマンド受信関数 (新規追加または既存の関数を置き換え) ---
void read_pc_serial_commands()
{
  if (USBSerial.available() > 0)
  {
    String command_str = USBSerial.readStringUntil('\n');
    command_str.trim(); // 前後の空白や改行を削除

    USBSerial.printf("[DRONE DEBUG] Received from PC: %s\n", command_str.c_str());

    // デバッグ用に受信文字列を表示
    // USBSerial.printf("Received from PC: %s\n", command_str.c_str());

    // スティック値を一時的に格納する変数
    float pc_yaw = 0.0f, pc_throttle = 0.0f, pc_roll = 0.0f, pc_pitch = 0.0f;
    int pc_arm = 0, pc_flip = 0, pc_flight_mode = ANGLECONTROL, pc_alt_mode = NOT_ALT_CONTROL_MODE;

    // 文字列をカンマで分割して解析
    int last_idx = 0;
    int comma_idx = 0;
    String pair_str;

    while ((comma_idx = command_str.indexOf(',', last_idx)) != -1 || last_idx < command_str.length())
    {
      if (comma_idx == -1 && last_idx < command_str.length())
      { // 最後の要素
        pair_str = command_str.substring(last_idx);
        last_idx = command_str.length(); // ループを終了させる
      }
      else
      {
        pair_str = command_str.substring(last_idx, comma_idx);
        last_idx = comma_idx + 1;
      }

      pair_str.trim();
      int colon_idx = pair_str.indexOf(':');
      if (colon_idx != -1)
      {
        String key = pair_str.substring(0, colon_idx);
        String value_str = pair_str.substring(colon_idx + 1);

        if (key.equals("Y"))
          pc_yaw = value_str.toFloat();
        else if (key.equals("T"))
          pc_throttle = value_str.toFloat();
        else if (key.equals("R"))
          pc_roll = value_str.toFloat();
        else if (key.equals("P"))
          pc_pitch = value_str.toFloat();
        else if (key.equals("ARM"))
          pc_arm = value_str.toInt();
        else if (key.equals("FLP"))
          pc_flip = value_str.toInt();
        else if (key.equals("MD"))
          pc_flight_mode = value_str.toInt();
        else if (key.equals("AM"))
          pc_alt_mode = value_str.toInt();
      }
    }

    // 解析した値を Stick 配列に格納
    // M5AtomS3コントローラーの Phi, Theta, Psi, Throttle は正規化されたfloat値であることに注意
    // PCからは -1.0～1.0 (ヨー、ロール、ピッチ)、0.0～1.0 (スロットル) の範囲で送ることを想定
    Stick[RUDDER] = pc_yaw;
    Stick[THROTTLE] = pc_throttle;
    Stick[AILERON] = pc_roll;
    Stick[ELEVATOR] = pc_pitch;

    Stick[BUTTON_ARM] = pc_arm;
    Stick[BUTTON_FLIP] = pc_flip;
    Stick[CONTROLMODE] = pc_flight_mode;

    // ALTCONTROLMODE は rc.hpp で定義されたインデックスを使用
    if (ALTCONTROLMODE < 16)
    { // 配列範囲チェック
      Stick[ALTCONTROLMODE] = pc_alt_mode;
    }
    USBSerial.printf("[DRONE DEBUG] Parsed Stick Values -> Thr:%.2f, Ail:%.2f, Ele:%.2f, Rud:%.2f, Arm:%d, AltM:%d\n",
                     Stick[THROTTLE], Stick[AILERON], Stick[ELEVATOR], Stick[RUDDER],
                     (int)Stick[BUTTON_ARM], (int)Stick[ALTCONTROLMODE]);

    Connect_flag = 0; // コマンド受信したので接続フラグをリセット
  }
}

void rc_init(void)
{
  // Initialize Stick list
  for (uint8_t i = 0; i < 16; i++)
    Stick[i] = 0.0;
  USBSerial.println("RC Ready for PC Serial Control.");
  // ESP-NOW初期化
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();

  // WiFi.macAddress((uint8_t*)MyMacAddr);
  // USBSerial.printf("MAC ADDRESS: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
  //                 MyMacAddr[0], MyMacAddr[1], MyMacAddr[2], MyMacAddr[3], MyMacAddr[4], MyMacAddr[5]);

  // if (esp_now_init() == ESP_OK) {
  //   USBSerial.println("ESPNow Init Success");
  // } else {
  //   USBSerial.println("ESPNow Init Failed");
  //   ESP.restart();
  // }

  // MACアドレスブロードキャスト
  //  uint8_t addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //  memcpy(peerInfo.peer_addr, addr, 6);
  //  peerInfo.channel = CHANNEL;
  //  peerInfo.encrypt = false;
  //  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  //  {
  //        USBSerial.println("Failed to add peer");
  //        return;
  //  }
  //  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);

  // //Send my MAC address
  // for (uint16_t i=0; i<20; i++)
  // {
  //   send_peer_info();
  //   delay(50);
  //   //USBSerial.printf("%d\n", i);
  // }

  // // ESP-NOW再初期化
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  // if (esp_now_init() == ESP_OK) {
  //   USBSerial.println("ESPNow Init Success2");
  // } else {
  //   USBSerial.println("ESPNow Init Failed2");
  //   ESP.restart();
  // }

  // //ペアリング
  // memcpy(peerInfo.peer_addr, TelemAddr, 6);
  // peerInfo.channel = CHANNEL;
  // peerInfo.encrypt = false;
  // if (esp_now_add_peer(&peerInfo) != ESP_OK)
  // {
  //       USBSerial.println("Failed to add peer2");
  //       return;
  // }
  // // ESP-NOWコールバック登録
  // esp_now_register_recv_cb(OnDataRecv);
  // esp_now_register_send_cb(on_esp_now_sent);
  // USBSerial.println("ESP-NOW Ready.");
}

void send_peer_info(void)
{
  uint8_t data[7];
  data[0] = CHANNEL;
  memcpy(&data[1], (uint8_t *)MyMacAddr, 6);
  esp_now_send(peerInfo.peer_addr, data, 7);
}

// uint8_t telemetry_send(uint8_t* data, uint16_t datalen)
// {
//   static uint32_t cnt=0;
//   static uint8_t error_flag = 0;
//   static uint8_t state=0;

//   esp_err_t result;

//   if ((error_flag == 0)&&(state==0))
//   {
//     result = esp_now_send(peerInfo.peer_addr, data, datalen);
//     cnt=0;
//   }
//   else cnt++;

//   if (esp_now_send_status == 0)
//   {
//     error_flag = 0;
//     //state = 0;
//   }
//   else
//   {
//     error_flag = 1;
//     //state = 1;
//   }
//   //一度送信エラーを検知してもしばらくしたら復帰する
//   if (cnt>500)
//   {
//     error_flag = 0;
//     cnt = 0;
//   }
//   cnt++;
//   //USBSerial.printf("%6d %d %d\r\n", cnt, error_flag, esp_now_send_status);

//   return error_flag;
// }

void rc_end(void)
{
  // Ps3.end();
}

uint8_t rc_isconnected(void)
{
  bool status;
  Connect_flag++;
  if (Connect_flag < 200)
    status = 1;
  else
    status = 0;
  // USBSerial.printf("%d \n\r", Connect_flag);
  return status;
}

void rc_demo()
{
}
