import queue
import time
import argparse
import cv2
import logging
import os
import threading


def setup_logging():
    log_dir = 'log'
    if not os.path.exists(log_dir):
        os.makedirs(log_dir)
    logging.basicConfig(filename=os.path.join(log_dir, 'camera.log'),
                        level=logging.ERROR,
                        format='%(asctime)s:%(levelname)s:%(message)s')


class Sensor:
    def get(self):
        raise NotImplementedError("Subclasses must implement method get")

# остальные датчики
class SensorX(Sensor):
    def __init__(self, delay: float):
        self.delay = delay
        self.data = 0

    def get(self) -> int:
        time.sleep(self.delay)
        self.data += 1
        return self.data

# датчик usb-камеры
class SensorCam(Sensor):
    def __init__(self, name, h, w):
        self.name = name
        self.h = h
        self.w = w
        self.cam = None
        self.cam_init()


    def cam_init(self):
        self.cam = cv2.VideoCapture(self.name)

        if not self.cam.isOpened():
            logging.error(f"Ошибка: Не удалось открыть камеру {self.name} была открыта стандартная камера")
            self.cam = cv2.VideoCapture(0)

        if not self.cam.isOpened():
            logging.error(f"Ошибка: Не удалось открыть камеру")
            exit(1)

        self.cam.set(cv2.CAP_PROP_FRAME_WIDTH, self.w)
        self.cam.set(cv2.CAP_PROP_FRAME_HEIGHT, self.h)


    def get(self):
        # Считываем кадр с камеры
        ret, frame = self.cam.read()
        if not ret:
            logging.error("Ошибка: Не удалось считать кадр.")
            exit(1)

        return frame

    def __del__(self):
        if self.cam is not None:
            self.cam.release()

# отображение картинки
class WindowImage:
    def __init__(self, frequency):
        self.frequency = frequency


    # def show(self, img):
    def show(self, img):
        cv2.imshow("Camera", img)
        cv2.waitKey(1)

    def __del__(self):
        cv2.destroyAllWindows()


def sensor_thread(sensor, queue_elem: queue.Queue, event_stop: threading.Event):
    while not event_stop.is_set():
        data = sensor.get()
        queue_elem.put(data)



def parse_arguments():
    parser = argparse.ArgumentParser(description="Video settings")
    parser.add_argument("--name", "-n", type=str, default="my_camera", help="camera name")
    parser.add_argument("--resolution", "-r", type=str, default="1400x1080", help="desired camera resolution")
    parser.add_argument("--frequency", "-f", type=int, default=100, help="image display frequency")
    return parser.parse_args()

def initialize_sensors():
    sensor0 = SensorX(0.01)
    sensor1 = SensorX(0.1)
    sensor2 = SensorX(1)
    return sensor0, sensor1, sensor2

def create_sensor_threads(sensors, queues, event_stop):
    threads = []
    for sensor, queue in zip(sensors, queues):
        thread = threading.Thread(target=sensor_thread, args=(sensor, queue, event_stop))
        threads.append(thread)
        thread.start()
    return threads

def update_sensor_values(queues, sensor_values):
    for i in range(len(queues)):
        while not queues[i].empty():
            item = queues[i].get()
            sensor_values[i] = item

def draw_text_on_frame(frame, sensor_values):
    text_lines = [
        f"Sensor0: {sensor_values[0]}",
        f"Sensor1: {sensor_values[1]}",
        f"Sensor2: {sensor_values[2]}"
    ]

    font = cv2.FONT_HERSHEY_SIMPLEX
    font_scale = 0.6
    thickness = 1
    text_color = (0, 0, 0)  # черный
    bg_color = (255, 255, 255)  # белый
    line_height = 25
    padding = 10

    max_width = max(cv2.getTextSize(line, font, font_scale, thickness)[0][0] for line in text_lines)
    box_width = max_width + 2 * padding
    box_height = line_height * len(text_lines) + 2 * padding

    x = frame.shape[1] - box_width - 10
    y = frame.shape[0] - box_height - 10

    cv2.rectangle(frame, (x, y), (x + box_width, y + box_height), bg_color, cv2.FILLED)

    for i, line in enumerate(text_lines):
        text_x = x + padding
        text_y = y + padding + (i + 1) * line_height - 8
        cv2.putText(frame, line, (text_x, text_y), font, font_scale, text_color, thickness, cv2.LINE_AA)

def main():
    setup_logging()
    args = parse_arguments()
    sensor0, sensor1, sensor2 = initialize_sensors()

    h, w = map(int, args.resolution.split('x'))
    frequency = args.frequency
    sensor_cam = SensorCam(args.name, h, w)
    window_img = WindowImage(frequency)

    queue_elems = [queue.Queue() for _ in range(3)]
    event_stop = threading.Event()

    sensors = [sensor0, sensor1, sensor2]
    threads = create_sensor_threads(sensors, queue_elems, event_stop)

    sensor_values = [0, 0, 0]
    try:
        while True:
            update_sensor_values(queue_elems, sensor_values)
            frame = sensor_cam.get()
            draw_text_on_frame(frame, sensor_values)
            window_img.show(frame)
            time.sleep(1 / frequency)

            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

    except KeyboardInterrupt:
        print("Завершение работы")

    finally:
        event_stop.set()
        for thread in threads:
            thread.join()
        del sensor_cam
        del window_img

    return 0

main()
