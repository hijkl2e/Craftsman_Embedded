int state[5];
int lastState[5];
unsigned long lastDebounceTime[5];
unsigned long debounceDelay = 50;

int mode = 0;
int num = -1;

/**
 * @brief 초기 핀 설정 및 스위치 상태 초기화 함수
 */
void setup() {
  for (int i = 2; i < 5; ++i) { // 스위치
    pinMode(i, INPUT);
  }
  for (int i = 5; i < 8; ++i) { // RGB LED
    pinMode(i, OUTPUT);
  }
  for (int i = 8; i < 12; ++i) { // BCD to 7-Segment 디코더
    pinMode(i, OUTPUT);
  }
  for (int i = 12; i < 14; ++i) { // 7-Segment LED
    pinMode(i, OUTPUT);
  }
}

/**
 * @brief 스위치 입력을 디바운싱 처리하여 상태 변화를 감지하는 함수
 *
 * 각 스위치의 입력을 확인하여 눌림/떼짐 상태 변화가
 * 안정적으로 발생했을 경우 해당 스위치의 핀 번호를 반환한다.
 *
 * @return 상태 변화가 감지된 스위치의 핀 번호,
 *         변화가 없으면 -1 반환
 */
int SWITCH() {
  for (int i = 2; i < 5; ++i) {
    int reading = digitalRead(i);
    if (lastState[i] != reading) {
      lastState[i] = reading;
      lastDebounceTime[i] = millis();
    } else if (millis() - lastDebounceTime[i] > debounceDelay) {
      if (state[i] != reading) {
        state[i] = reading;
        return i;
      }
    }
  }
  return -1;
}

/**
 * @brief 스위치 상태 초기화 함수
 */
void CLEAR_SWITCH() {
  for (int i = 2; i < 5; ++i) {
    state[i] = HIGH;
    lastState[i] = HIGH;
    lastDebounceTime[i] = 0;
  }
}

/**
 * @brief LED 출력 제어 함수
 *
 * 지정된 번호에 해당하는 LED만 점등하고 나머지는 소등한다.
 *
 * @param c LED 인덱스 (1: R, 2: G, 3: B)
 */
void LED(int c) {
  for (int i = 5; i < 8; ++i) {
    digitalWrite(i, c == (i - 4));
  }
}

/**
 * @brief 1자리 숫자를 FND에 표시하는 함수
 *
 * 두 FND에 동일한 1자리 숫자를 표시한다.
 *
 * @param n 표시할 숫자 (-1이면 소등)
 */
void DIGIT1(int n) {
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  if (n == -1) {
    return;
  }
  num = 11 * n;
  for (int i = 8; i < 12; ++i) {
    digitalWrite(i, (n >> (i - 8)) & 1);
  }
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
}

/**
 * @brief 2자리 숫자를 FND에 표시하는 함수
 *
 * 2자리 숫자를 멀티플렉싱 방식으로 표시한다.
 *
 * @param n 표시할 숫자
 */
void DIGIT2(int n) {
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  num = n;
  for (int k = 12; k < 14; ++k) {
    int m = k == 12 ? (n / 10) : (n % 10);
    for (int i = 8; i < 12; ++i) {
      digitalWrite(i, (m >> (i - 8)) & 1);
    }
    digitalWrite(k, HIGH);
    delay(10);
    digitalWrite(k, LOW);
  }
}

/**
 * [초기화 동작]
 * - 전원을 최초로 인가하거나 [동작 1] 또는 [동작 2]를 수행하는 도중 SW3을 누르는
 *   경우 다음의 [초기화 동작]을 수행하도록 합니다.
 *  1) FND에 숫자 88을 1초 점등, 1초 소등을 2회 반복한 다음 숫자 00을 표시합니다.
 *  2) RGB LED 모듈은 순서대로 R(적색)이 1초, G(녹색)가 1초, B(청색)가 1초 점등한 뒤
 *     1초 소등한 다음 R(적색)이 점등합니다.
 *  3) 수행이 완료한 상태를 [초기화 상태]로 하여 숫자 00과 아두이노 RGB LED 모듈의
 *     R(적색) 점등 상태를 유지합니다.
 *  4) [초기화 동작]을 수행하는 도중에는 어떠한 스위치 입력도 무시합니다.
 */
int INIT() {
  for (int i = 1; i < 5; ++i) {
    DIGIT1(i % 2 ? 8 : -1);
    LED(i);
    delay(1000);
  }
  DIGIT1(0);
  LED(1);
  while (true) {
    int s = SWITCH();
    if ((s == 2 || s == 3) && state[s]) {
      return s - 1;
    }
  }
}

/**
 * [동작 1]
 * - [초기화 상태]이거나 [동작 2]를 수행 중일 때 SW1을 눌렀다 떼면 즉시 다음의
 *   [동작 1]을 수행하도록 합니다.
 *  1) FND는 88을 2초간 표시하고, RGB LED 모듈은 G(녹색)를 0.5초간 점등한 후 R(적색)을
 *     0.5초간 점등한다. 이후 아래 동작을 수행합니다.
 *  2) RGB LED 모듈은 0.5초간 B(청색)를 점등하고 0.5초간 소등하는 동작을 반복합니다.
 *  3) FND는 00에서 99까지 0.5초에 표시하는 숫자를 1씩 증가하는 동작을 반복합니다.
 */
int MODE1() {
  unsigned long time = millis();
  for (int i = -4;;) {
    if (i < 0) {
      DIGIT1(8);
    } else {
      DIGIT2(i % 100);
    }
    if (i == -4) {
      LED(2);
    } else if (i == -3) {
      LED(1);
    } else {
      LED(i % 2 ? 0 : 3);
    }
    int s = SWITCH();
    if (s == 4 && !state[s]) {
      return 0;
    } else if (s == 3 && state[s]) {
      return 2;
    } else if (s == 2 && state[s]) {
      MODE3();
      time = millis();
    }
    if (millis() - time >= 500) {
      time += 500;
      ++i;
    }
  }
}

/**
 * [동작 2]
 * - [초기화 상태]이거나 [동작 1]를 수행 중일 때 SW2를 눌렀다 떼면 즉시 다음의
 *   [동작 2]을 수행하도록 합니다.
 *  1) FND는 88을 2초간 표시하고, RGB LED 모듈은 B(청색)를 0.5초간 점등한 후 R(적색)을
 *     0.5초간 점등한다. 이후 아래 동작을 수행합니다.
 *  2) RGB LED 모듈은 0.5초간 G(녹색)를 점등하고 0.5초간 소등하는 동작을 반복합니다.
 *  3) FND는 99에서 0까지 0.5초에 1씩 표시하는 숫자를 감소하는 동작을 반복합니다. 
 */
int MODE2() {
  unsigned long time = millis();
  for (int i = -4;;) {
    if (i < 0) {
      DIGIT1(8);
    } else {
      DIGIT2(99 - i % 100);
    }
    if (i == -4) {
      LED(3);
    } else if (i == -3) {
      LED(1);
    } else {
      LED(i % 2 ? 0 : 2);
    }
    int s = SWITCH();
    if (s == 4 && !state[s]) {
      return 0;
    } else if (s == 2 && state[s]) {
      return 1;
    } else if (s == 3 && state[s]) {
      MODE3();
      time = millis();
    }
    if (millis() - time >= 500) {
      time += 500;
      ++i;
    }
  }
}

/**
 * [동작 3]
 * - [동작 1]을 수행 중일 때 SW1을, 또는 [동작 2]를 수행 중일 때 SW2를 눌렀다 떼면
 *   수행 중인 동작을 일시 중지하고, 아래의 [동작 3]을 수행하도록 합니다. [동작 3]의
 *   수행을 완료한 뒤 앞서 수행하던 동작을 이어서 진행하도록 합니다.
 *  1) FND에 현재 표시 중인 숫자를 2초 점등, 2초 소등을 2회 반복하고 RGB LED 모듈은
 *     R(적색)을 2초 점등, 2초 소등을 2회 반복합니다.
 *  2) [동작 3]을 수행하는 도중에는 어떠한 스위치 입력도 무시합니다.
 */
void MODE3() {
  for (int i = 0; i < 2; ++i) {
    LED(1);
    unsigned long time = millis();
    while (true) {
      DIGIT2(num);
      if (millis() - time >= 2000) {
        break;
      }
    }
    LED(0);
    DIGIT1(-1);
    delay(2000);
  }
  CLEAR_SWITCH();
}

/**
 * @brief 메인 루프 함수
 *
 * 현재 설정된 mode 값에 따라
 * INIT, MODE1, MODE2 중 하나를 실행한다.
 */
void loop() {
  CLEAR_SWITCH();
  if (mode == 0) {
    mode = INIT();
  } else if (mode == 1) {
    mode = MODE1();
  } else if (mode == 2) {
    mode = MODE2();
  }
}
