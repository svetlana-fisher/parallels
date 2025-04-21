import queue
import threading
import time
import cv2 as cv
import argparse
from ultralytics import YOLO
from ultralytics.utils import ThreadingLocked


class Video:
    def __init__(self, video_path, output_path):
        self.cap = cv.VideoCapture(video_path)
        if not self.cap.isOpened():
            raise Exception("Could not open video.")
        fourcc = cv.VideoWriter_fourcc(*'mp4v')
        width = int(self.cap.get(cv.CAP_PROP_FRAME_WIDTH))
        height = int(self.cap.get(cv.CAP_PROP_FRAME_HEIGHT))
        self.out = cv.VideoWriter(output_path, fourcc, 20.0, (width, height))

    def get(self):
        ret, frame = self.cap.read()
        if not ret:
            return None
        return frame

    def save(self, frame):
        if frame is not None:
            self.out.write(frame)


    def __del__(self):
        self.cap.release()
        self.out.release()
        cv.destroyAllWindows()


def model_process(frame):
    model = YOLO('yolov8s-pose.pt')
    results = model(frame)
    if results:
        annotated_frame = results[0].plot()
        return annotated_frame
    return frame


def worker(queue_input, queue_output, event_stop):

    model = YOLO('yolov8s-pose.pt').to("cpu")
    while not event_stop.is_set():
        try:
            frame, frame_idx = queue_input.get()
            # print(frame_idx)
            if frame is None:
                break
            processed_frame = model.predict(frame, verbose=False)
            queue_output.put((processed_frame[0].plot(labels=False, boxes=False), frame_idx))
        except queue.Empty:
            continue

def parse_args():
    parser = argparse.ArgumentParser(description="settings")
    parser.add_argument("--path", "-p", type=str, required=True, help="video path")
    parser.add_argument("--mode", "-m", type=str, choices=["single_threaded", "multithreaded"], required=True, help="execution mode")
    parser.add_argument("--name", "-n", type=str, required=True, help="output file name")
    return parser.parse_args()

def main():
    args = parse_args()
    video_path = args.path
    mode = args.mode
    output_name = args.name

    video_obj = Video(video_path, output_name)
    queue_input = queue.Queue()
    queue_output = queue.Queue()
    event_stop = threading.Event()

    if mode == "multithreaded":
        num_threads = 16
    else:
        num_threads = 1

    threads = []
    start_time = time.time()
    frame_idx = 0
    processed_frames = {}

    while True:
        frame = video_obj.get()
        if frame is None:
            break
        queue_input.put((frame, frame_idx))
        frame_idx += 1

    for _ in range(num_threads):
        queue_input.put((None, None))

    for _ in range(num_threads):
        thread = threading.Thread(target=worker, args=(queue_input, queue_output, event_stop))
        thread.start()
        threads.append(thread)

    while not queue_input.empty() or any(t.is_alive() for t in threads):
        try:
            processed_frame, frame_idx = queue_output.get(timeout=0.1)
            processed_frames[frame_idx] = processed_frame
            # video_obj.save(processed_frame)
        except queue.Empty:
            continue

    event_stop.set()
    for thread in threads:
        thread.join()

    for idx in sorted(processed_frames.keys()):
        video_obj.save(processed_frames[idx])

    end_time = time.time()
    print(f"Время выполнения: {end_time - start_time:.2f} секунд")

if __name__ == "__main__":
    main()
