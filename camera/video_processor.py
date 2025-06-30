# Copyright (c) 2025 by T3 Foundation. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#     https://docs.t3gemstone.org/en/license
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import time

import cv2
import numpy as np


class FaceTracker:
    def __init__(self, colors: dict, model_dir: str, min_confidence: float = 0.7):
        self.m_colors = colors
        self.m_min_confidence = min_confidence
        self.m_face_count = 0
        self.m_model_dir = model_dir

        model_path = os.path.join(self.m_model_dir, "face_detection_yunet_2023mar_int8.onnx")
        self.m_face_net = cv2.FaceDetectorYN.create(
            model=model_path,
            config="",
            input_size=(320, 320),
            score_threshold=min_confidence,
            nms_threshold=0.3,
            top_k=5000,
            backend_id=cv2.dnn.DNN_BACKEND_DEFAULT,
            target_id=cv2.dnn.DNN_TARGET_CPU,
        )

        if cv2.cuda.getCudaEnabledDeviceCount() > 0:
            self.m_face_net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
            self.m_face_net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA)

    def detect_and_enhance_faces(self, frame: np.ndarray) -> np.ndarray:
        (h, w) = frame.shape[:2]

        self.m_face_net.setInputSize((w, h))
        _, detections = self.m_face_net.detect(frame)

        final_boxes = []

        if detections is not None:
            for detection in detections:
                x1, y1, w_box, h_box = detection[:4].astype(int)
                confidence = detection[-1]

                # Calculate box coordinates
                startX, startY = x1, y1
                endX, endY = x1 + w_box, y1 + h_box

                # Validate box dimensions
                startX, startY = max(0, startX), max(0, startY)
                endX, endY = min(w - 1, endX), min(h - 1, endY)
                width, height = endX - startX, endY - startY

                # Filter by aspect ratio
                if width > 0 and height > 0 and 0.5 < width / height < 2.0:
                    final_boxes.append((startX, startY, endX, endY))

                    label = f"{confidence * 100:.1f}%"
                    cv2.putText(
                        frame, label, (startX, startY - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, self.m_colors["faces"], 2
                    )

        for startX, startY, endX, endY in final_boxes:
            cv2.rectangle(frame, (startX, startY), (endX, endY), self.m_colors["faces"], 2)

        self.m_face_count = len(final_boxes)
        return frame


class VideoProcessor:

    def __init__(self, model_dir: str, video_path: str = None, camera_index: int = None, snapshot_dir: str = None):
        if video_path is None and camera_index is None:
            raise ValueError("Either video_path or camera_index must be provided")
        if video_path is not None and camera_index is not None:
            raise ValueError("Only one of video_path or camera_index should be provided")
        if model_dir is None:
            raise ValueError("model_dir should be provided")

        self.m_video_path = video_path
        self.m_camera_index = camera_index
        self.m_model_dir = model_dir
        self.m_snapshot_dir = snapshot_dir if snapshot_dir else "snapshots"
        self.m_cap = None
        self.m_frame_count = 0
        self.m_fps_counter = 0
        self.m_fps_start_time = time.time()
        self.m_current_fps = 0
        self.m_is_camera = camera_index is not None
        self.m_processed_frame = np.zeros((1, 1, 1), dtype=np.uint8)

        self.m_frame_duration = 1.0 / 30.0  # Default to 30 FPS, will be updated
        self.m_last_frame_time = 0

        self.m_effects = {"faces": True, "info_overlay": True}
        self.m_colors = {"faces": (100, 255, 255)}

        self.m_face_tracker = FaceTracker(self.m_colors, self.m_model_dir)

        os.makedirs(self.m_snapshot_dir, exist_ok=True)

    def __del__(self):
        if self.m_cap:
            self.m_cap.release()
        cv2.destroyAllWindows()

    def _initialize_video(self) -> int:
        if self.m_is_camera:
            self.m_cap = cv2.VideoCapture(self.m_camera_index)
            if not self.m_cap.isOpened():
                print(f"Error: Could not open camera {self.m_camera_index}", file=sys.stderr)
                return 1

            print(f"Camera {self.m_camera_index} initialized")
        else:
            self.m_cap = cv2.VideoCapture(self.m_video_path)
            if not self.m_cap.isOpened():
                print(f"Error: Could not open video file {self.m_video_path}", file=sys.stderr)
                return 1

            self.m_total_frames = int(self.m_cap.get(cv2.CAP_PROP_FRAME_COUNT))
            self.m_video_fps = self.m_cap.get(cv2.CAP_PROP_FPS)

            if self.m_video_fps > 0:
                self.m_frame_duration = 1.0 / self.m_video_fps
            else:
                self.m_frame_duration = 1.0 / 30.0  # Fallback to 30 FPS

            print(f"Video loaded: {self.m_total_frames} frames at {self.m_video_fps} FPS")
            print(f"Frame duration: {self.m_frame_duration:.4f} seconds")

        self.m_last_frame_time = time.time()
        return 0

    def _calculate_fps(self):
        self.m_fps_counter += 1
        if self.m_fps_counter >= 30:
            current_time = time.time()
            self.m_current_fps = 30 / (current_time - self.m_fps_start_time)
            self.m_fps_start_time = current_time
            self.m_fps_counter = 0

    def _wait_for_frame_timing(self):
        # For camera, we don't need to wait since we want real-time processing
        if self.m_is_camera:
            return

        current_time = time.time()
        elapsed_time = current_time - self.m_last_frame_time

        if elapsed_time < self.m_frame_duration:
            sleep_time = self.m_frame_duration - elapsed_time
            time.sleep(sleep_time)

        self.m_last_frame_time = time.time()

    def _draw_info_overlay(self, frame: np.ndarray):
        if not self.m_effects["info_overlay"]:
            return

        h, w = frame.shape[:2]
        overlay = frame.copy()

        overlay_width = 220
        overlay_height = 280

        # Semi-transparent background with gradient effect
        cv2.rectangle(overlay, (10, 10), (overlay_width, overlay_height), (0, 0, 0), -1)
        cv2.addWeighted(overlay, 0.75, frame, 0.25, 0, frame)

        # Border
        cv2.rectangle(frame, (10, 10), (overlay_width, overlay_height), (0, 255, 255), 1)

        # Text information
        font = cv2.FONT_HERSHEY_SIMPLEX
        small_font = 0.4
        medium_font = 0.5
        large_font = 0.6

        # Header
        header_text = "Video Processor"
        cv2.putText(frame, header_text, (20, 35), font, large_font, (0, 255, 255), 2)

        # Source info
        y_pos = 60
        source_text = f"Camera {self.m_camera_index}" if self.m_is_camera else "Video File"
        cv2.putText(frame, f"Source: {source_text}", (25, y_pos), font, small_font, (255, 255, 255), 1)

        # Performance metrics
        y_pos += 25
        cv2.putText(frame, "Performance", (20, y_pos), font, medium_font, (100, 255, 100), 1)
        y_pos += 20
        cv2.putText(frame, f"Display FPS: {self.m_current_fps:.1f}", (25, y_pos), font, small_font, (255, 255, 255), 1)
        if not self.m_is_camera:
            y_pos += 15
            cv2.putText(frame, f"Source FPS: {self.m_video_fps:.1f}", (25, y_pos), font, small_font, (255, 255, 255), 1)

        if not self.m_is_camera:
            y_pos += 25
            cv2.putText(frame, "Progress", (20, y_pos), font, medium_font, (100, 255, 100), 1)
            y_pos += 20
            progress_text = f"{self.m_frame_count}/{self.m_total_frames}"
            cv2.putText(frame, progress_text, (25, y_pos), font, small_font, (255, 255, 255), 1)

            # Progress bar
            y_pos += 10
            bar_width = overlay_width - 50
            bar_height = 8
            progress_ratio = self.m_frame_count / max(self.m_total_frames, 1)

            cv2.rectangle(frame, (25, y_pos), (25 + bar_width, y_pos + bar_height), (50, 50, 50), -1)
            cv2.rectangle(
                frame, (25, y_pos), (25 + int(bar_width * progress_ratio), y_pos + bar_height), (0, 255, 255), -1
            )
        else:
            # Frame counter for camera
            y_pos += 25
            cv2.putText(frame, "Frames", (20, y_pos), font, medium_font, (100, 255, 100), 1)
            y_pos += 20
            cv2.putText(frame, f"Processed: {self.m_frame_count}", (25, y_pos), font, small_font, (255, 255, 255), 1)

        # Detection info
        y_pos += 30
        cv2.putText(frame, "Detection", (20, y_pos), font, medium_font, (100, 255, 100), 1)
        y_pos += 20

        # Count detected objects
        face_count = self.m_face_tracker.m_face_count

        cv2.putText(frame, f"Faces: {face_count}", (25, y_pos), font, small_font, self.m_colors["faces"], 1)

        # Effects status
        y_pos += 25
        cv2.putText(frame, "Effects", (20, y_pos), font, medium_font, (100, 255, 100), 1)
        y_pos += 5

        for effect, enabled in self.m_effects.items():
            if effect != "info_overlay":
                y_pos += 15
                color = (0, 255, 0) if enabled else (0, 0, 255)
                status = "ON" if enabled else "OFF"
                effect_name = effect.replace("_", " ").title()
                cv2.putText(frame, f"{effect_name}: {status}", (25, y_pos), font, small_font, color, 1)

    def _process_frame(self, frame: np.ndarray) -> np.ndarray:
        processed_frame = frame.copy()

        if self.m_effects["faces"]:
            processed_frame = self.m_face_tracker.detect_and_enhance_faces(processed_frame)

        self._draw_info_overlay(processed_frame)

        return processed_frame

    def _handle_keyboard_input(self, key: int) -> int:
        if key == ord("q"):
            return 1
        elif key == ord("f"):
            self.m_effects["faces"] = not self.m_effects["faces"]
            self.m_face_tracker.m_face_count = 0
            print(f"Face tracking: {'ON' if self.m_effects['faces'] else 'OFF'}")
        elif key == ord("i"):
            self.m_effects["info_overlay"] = not self.m_effects["info_overlay"]
            print(f"Info overlay: {'ON' if self.m_effects['info_overlay'] else 'OFF'}")
        elif key == ord("r") and not self.m_is_camera:
            # Reset only works for video files
            self.m_cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
            self.m_frame_count = 0
            self.m_last_frame_time = time.time()  # Reset timing
            print("Video reset to beginning")
        elif key == ord("s"):
            timestamp = int(time.time())
            filename = os.path.join(self.m_snapshot_dir, f"snapshot_{timestamp}.jpg")
            cv2.imwrite(filename, self.m_processed_frame)
            print(f"Snapshot saved as {filename}")

        return 0

    def run(self):
        if self._initialize_video():
            return

        print("\nVideo Processor Controls:")
        print("- 'q': Quit")
        print("- 'f': Toggle face tracking")
        print("- 'i': Toggle info overlay")
        print("- 's': Save snapshot")

        if not self.m_is_camera:
            print("- 'r': Reset video to beginning")

        window_title = "Video Processor"
        cv2.namedWindow(window_title, cv2.WINDOW_NORMAL)

        try:
            while True:
                ret, frame = self.m_cap.read()

                if not ret:
                    if self.m_is_camera:
                        print("Error reading from camera")
                        break
                    else:
                        print("End of video reached, looping...")
                        self.m_cap.set(cv2.CAP_PROP_POS_FRAMES, 0)
                        self.m_frame_count = 0
                        self.m_last_frame_time = time.time()
                        continue

                self.m_frame_count += 1

                self.m_processed_frame = self._process_frame(frame)

                self._calculate_fps()

                cv2.imshow(window_title, self.m_processed_frame)

                self._wait_for_frame_timing()

                key = cv2.waitKey(1) & 0xFF
                if self._handle_keyboard_input(key):
                    print("\nShutting down...")
                    break

        except KeyboardInterrupt:
            print("\nShutting down...")
