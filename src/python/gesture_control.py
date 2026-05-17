"""Laptop-side gesture controller for the STM32 Gesture-Controlled Smart Home project.

This program uses OpenCV and MediaPipe to read hand gestures from a webcam,
converts those gestures into compact UART commands, and sends the commands to
an STM32 microcontroller. The STM32 firmware handles the OLED menu, LED PWM,
fan control, and servo door movement.

Author: Troy Sengmany
"""

import cv2
import mediapipe as mp
import serial
import time
import math
import serial.tools.list_ports


# ---------------- SERIAL ----------------
def find_stm_port():
    ports = list(serial.tools.list_ports.comports())

    for port in ports:
        device = port.device.lower()
        description = (port.description or "").lower()
        manufacturer = (port.manufacturer or "").lower()

        if "usbmodem" in device:
            return port.device
        if "stlink" in description or "stm" in description:
            return port.device
        if "stmicro" in manufacturer:
            return port.device

    raise RuntimeError("STM32 serial port not found. Check USB connection.")


PORT = find_stm_port()
print("Using port:", PORT)

ser = serial.Serial(PORT, 38400, timeout=0)
time.sleep(0.6)


# ---------------- MEDIAPIPE ----------------
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)


# ---------------- MODES ----------------
mode = "menu"
menu_items = ["Lights", "Fan", "Door"]
menu_index = 0

selected_light = 1
last_sent = {1: -1, 2: -1, 3: -1}

door_open = False
last_door_command = None

fan_speed = 0
fan_locked = False
last_fan_speed_sent = -1


# ---------------- TIMING ----------------
last_swipe_time = 0
last_door_time = 0
last_fan_time = 0
last_fan_gesture_time = 0

SWIPE_COOLDOWN = 0.60
DOOR_COOLDOWN = 0.35
FAN_SEND_COOLDOWN = 0.06
FAN_GESTURE_COOLDOWN = 0.45

back_hold_start = None
back_gesture_active = False
BACK_HOLD_TIME = 0.75

SWIPE_THRESHOLD_X = 0.045
SWIPE_THRESHOLD_Y = 0.045

FAN_STEP = 25

prev_center_x = None
prev_center_y = None


# ---------------- HELPERS ----------------
def send_command(cmd):
    ser.write((cmd + "\n").encode())
    print("Sent:", cmd)


def map_value(x, in_min, in_max, out_min, out_max):
    x = max(in_min, min(x, in_max))
    return int((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)


def clamp(val, low, high):
    return max(low, min(val, high))


def is_finger_up(hand_landmarks, tip_idx, pip_idx):
    return hand_landmarks.landmark[tip_idx].y < hand_landmarks.landmark[pip_idx].y


def fingers_up(hand_landmarks):
    count = 0

    thumb_tip_x = hand_landmarks.landmark[4].x
    thumb_ip_x = hand_landmarks.landmark[3].x

    if thumb_tip_x < thumb_ip_x:
        count += 1

    if is_finger_up(hand_landmarks, 8, 6):
        count += 1
    if is_finger_up(hand_landmarks, 12, 10):
        count += 1
    if is_finger_up(hand_landmarks, 16, 14):
        count += 1
    if is_finger_up(hand_landmarks, 20, 18):
        count += 1

    return count


def left_select_count(hand_landmarks):
    count = 0

    if is_finger_up(hand_landmarks, 8, 6):
        count += 1
    if is_finger_up(hand_landmarks, 12, 10):
        count += 1
    if is_finger_up(hand_landmarks, 16, 14):
        count += 1

    return count


def hand_center(hand_landmarks):
    wrist = hand_landmarks.landmark[0]
    middle_mcp = hand_landmarks.landmark[9]

    return (wrist.x + middle_mcp.x) / 2.0, (wrist.y + middle_mcp.y) / 2.0


def check_back_hold(finger_count, now):
    global back_hold_start
    global back_gesture_active

    if finger_count == 4:
        back_gesture_active = True

        if back_hold_start is None:
            back_hold_start = now

        if now - back_hold_start >= BACK_HOLD_TIME:
            back_hold_start = None
            back_gesture_active = False
            return True
    else:
        back_hold_start = None
        back_gesture_active = False

    return False


def send_fan_speed(speed, force=False):
    global fan_speed
    global last_fan_speed_sent
    global last_fan_time

    now = time.time()
    speed = clamp(speed, 0, 999)

    if force or (
        now - last_fan_time >= FAN_SEND_COOLDOWN
        and abs(speed - last_fan_speed_sent) >= FAN_STEP
    ):
        send_command(f"FAN:{speed}")
        fan_speed = speed
        last_fan_speed_sent = speed
        last_fan_time = now


# ---------------- MAIN PROGRAM ----------------
try:
    with mp_hands.Hands(
        static_image_mode=False,
        max_num_hands=2,
        model_complexity=0,
        min_detection_confidence=0.7,
        min_tracking_confidence=0.7
    ) as hands:

        while True:
            ret, frame = cap.read()

            if not ret:
                break

            frame = cv2.flip(frame, 1)
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            results = hands.process(rgb)

            left_hand = None
            right_hand = None

            now = time.time()
            h, w, _ = frame.shape

            if results.multi_hand_landmarks and results.multi_handedness:
                for hand_landmarks, handedness in zip(
                    results.multi_hand_landmarks,
                    results.multi_handedness
                ):
                    label = handedness.classification[0].label

                    mp_draw.draw_landmarks(
                        frame,
                        hand_landmarks,
                        mp_hands.HAND_CONNECTIONS
                    )

                    if label == "Left":
                        left_hand = hand_landmarks
                    elif label == "Right":
                        right_hand = hand_landmarks

            # ---------------- MENU MODE ----------------
            if mode == "menu":
                control_hand = right_hand if right_hand is not None else left_hand

                if control_hand is not None:
                    center_x, center_y = hand_center(control_hand)

                    if prev_center_x is not None and prev_center_y is not None:
                        dx = center_x - prev_center_x
                        dy = center_y - prev_center_y

                        if now - last_swipe_time > SWIPE_COOLDOWN:
                            if abs(dy) > abs(dx):
                                if dy < -SWIPE_THRESHOLD_Y:
                                    menu_index = (menu_index - 1) % len(menu_items)
                                    send_command("UP")
                                    last_swipe_time = now

                                elif dy > SWIPE_THRESHOLD_Y:
                                    menu_index = (menu_index + 1) % len(menu_items)
                                    send_command("DOWN")
                                    last_swipe_time = now

                            else:
                                if dx > SWIPE_THRESHOLD_X:
                                    send_command("SELECT")
                                    time.sleep(0.15)

                                    if menu_items[menu_index] == "Lights":
                                        mode = "lights"
                                        send_command(f"LIGHT:{selected_light}")

                                    elif menu_items[menu_index] == "Fan":
                                        mode = "fan"

                                    elif menu_items[menu_index] == "Door":
                                        mode = "door"

                                    last_swipe_time = now
                                    prev_center_x = None
                                    prev_center_y = None

                    prev_center_x = center_x
                    prev_center_y = center_y

                else:
                    prev_center_x = None
                    prev_center_y = None

                cv2.putText(frame, "MODE: MENU", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

                for i, item in enumerate(menu_items):
                    prefix = ">" if i == menu_index else " "
                    color = (0, 255, 255) if i == menu_index else (200, 200, 200)

                    cv2.putText(frame, f"{prefix} {item}", (10, 70 + i * 30),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.75, color, 2)

                cv2.putText(frame, "Swipe up/down = move | swipe right = select",
                            (10, 170), cv2.FONT_HERSHEY_SIMPLEX, 0.6,
                            (255, 255, 255), 2)

            # ---------------- LIGHTS MODE ----------------
            elif mode == "lights":
                prev_center_x = None
                prev_center_y = None

                left_count = 0
                brightness = None
                right_count = None

                if left_hand is not None:
                    left_count = left_select_count(left_hand)

                    if left_count == 1:
                        selected_light = 1
                    elif left_count == 2:
                        selected_light = 2
                    elif left_count == 3:
                        selected_light = 3

                if right_hand is not None:
                    right_count = fingers_up(right_hand)

                    if check_back_hold(right_count, now):
                        send_command("BACK")
                        mode = "menu"
                        menu_index = 0
                        last_sent = {1: -1, 2: -1, 3: -1}
                        continue

                    if not back_gesture_active:
                        thumb_tip = right_hand.landmark[4]
                        index_tip = right_hand.landmark[8]

                        x1, y1 = int(thumb_tip.x * w), int(thumb_tip.y * h)
                        x2, y2 = int(index_tip.x * w), int(index_tip.y * h)

                        distance = math.hypot(x2 - x1, y2 - y1)

                        if distance < 55:
                            brightness = 0
                        elif distance > 150:
                            brightness = 999
                        else:
                            brightness = map_value(distance, 55, 150, 0, 999)

                        cv2.circle(frame, (x1, y1), 8, (255, 0, 255), -1)
                        cv2.circle(frame, (x2, y2), 8, (255, 0, 255), -1)
                        cv2.line(frame, (x1, y1), (x2, y2), (0, 255, 0), 3)

                        if abs(brightness - last_sent[selected_light]) > 10 or brightness == 0 or brightness == 999:
                            send_command(f"L{selected_light}:{brightness}")
                            last_sent[selected_light] = brightness

                cv2.putText(frame, "MODE: LIGHTS", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

                cv2.putText(frame, f"Selected Light: {selected_light}", (10, 70),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)

                cv2.putText(frame, f"Left Count: {left_count}", (10, 110),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)

                if brightness is not None:
                    cv2.putText(frame, f"Brightness: {brightness}", (10, 150),
                                cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

                cv2.putText(frame, "Right pinch = dim | Hold 4 = Back",
                            (10, 190), cv2.FONT_HERSHEY_SIMPLEX, 0.65,
                            (255, 255, 255), 2)

            # ---------------- FAN MODE ----------------
            elif mode == "fan":
                prev_center_x = None
                prev_center_y = None

                left_count = 0
                right_count = None
                pinch_speed = None

                if left_hand is not None:
                    left_count = left_select_count(left_hand)

                    if now - last_fan_gesture_time > FAN_GESTURE_COOLDOWN:
                        if left_count == 1:
                            fan_locked = False
                            last_fan_gesture_time = now

                        elif left_count == 2:
                            fan_locked = True
                            last_fan_gesture_time = now

                        elif left_count == 3:
                            fan_locked = True
                            send_fan_speed(0, force=True)
                            last_fan_gesture_time = now

                if right_hand is not None:
                    right_count = fingers_up(right_hand)

                    if check_back_hold(right_count, now):
                        send_command("BACK")
                        mode = "menu"
                        menu_index = 0
                        continue

                    if not fan_locked and not back_gesture_active:
                        thumb_tip = right_hand.landmark[4]
                        index_tip = right_hand.landmark[8]

                        x1, y1 = int(thumb_tip.x * w), int(thumb_tip.y * h)
                        x2, y2 = int(index_tip.x * w), int(index_tip.y * h)

                        distance = math.hypot(x2 - x1, y2 - y1)

                        if distance < 55:
                            pinch_speed = 0
                        elif distance > 150:
                            pinch_speed = 999
                        else:
                            pinch_speed = map_value(distance, 55, 150, 0, 999)

                        send_fan_speed(pinch_speed)

                        cv2.circle(frame, (x1, y1), 8, (255, 0, 255), -1)
                        cv2.circle(frame, (x2, y2), 8, (255, 0, 255), -1)
                        cv2.line(frame, (x1, y1), (x2, y2), (0, 255, 0), 3)

                cv2.putText(frame, "MODE: FAN", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

                cv2.putText(frame, f"Fan Speed: {fan_speed}", (10, 70),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 255), 2)

                cv2.putText(frame, "LOCKED" if fan_locked else "UNLOCKED",
                            (10, 105), cv2.FONT_HERSHEY_SIMPLEX, 0.75,
                            (0, 0, 255) if fan_locked else (0, 255, 0), 2)

                cv2.putText(frame, "Left 1=Unlock  Left 2=Lock  Left 3=Off",
                            (10, 145), cv2.FONT_HERSHEY_SIMPLEX, 0.6,
                            (255, 255, 255), 2)

                cv2.putText(frame, "Right pinch=Speed | Hold 4=Back",
                            (10, 180), cv2.FONT_HERSHEY_SIMPLEX, 0.6,
                            (255, 255, 255), 2)

            # ---------------- DOOR MODE ----------------
            elif mode == "door":
                prev_center_x = None
                prev_center_y = None

                control_hand = right_hand if right_hand is not None else left_hand
                finger_count = None

                if control_hand is not None:
                    finger_count = fingers_up(control_hand)

                    if check_back_hold(finger_count, now):
                        send_command("BACK")
                        mode = "menu"
                        menu_index = 0
                        continue

                    if (
                        finger_count == 1
                        and last_door_command != "DOOR:1"
                        and now - last_door_time > DOOR_COOLDOWN
                    ):
                        send_command("DOOR:1")
                        door_open = False
                        last_door_command = "DOOR:1"
                        last_door_time = now

                    elif (
                        finger_count == 2
                        and last_door_command != "DOOR:0"
                        and now - last_door_time > DOOR_COOLDOWN
                    ):
                        send_command("DOOR:0")
                        door_open = True
                        last_door_command = "DOOR:0"
                        last_door_time = now

                cv2.putText(frame, "MODE: DOOR", (10, 30),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

                cv2.putText(frame, "1 finger = CLOSE", (10, 70),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), 2)

                cv2.putText(frame, "2 fingers = OPEN", (10, 105),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), 2)

                cv2.putText(frame, "Hold 4 fingers = Back", (10, 140),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.75, (255, 255, 255), 2)

            cv2.imshow("Smart Home Hand Tracking", frame)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                break

finally:
    ser.write(b"L1:0\n")
    ser.write(b"L2:0\n")
    ser.write(b"L3:0\n")
    ser.write(b"FAN:0\n")
    time.sleep(0.05)

    cap.release()
    cv2.destroyAllWindows()
    ser.close()