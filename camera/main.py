#!.venv/bin/python3
import argparse

from video_processor import VideoProcessor


def main():
    parser = argparse.ArgumentParser(description="Video Processor")

    source_group = parser.add_mutually_exclusive_group(required=True)
    source_group.add_argument("-v", "--video-path", help="Path to video file")
    source_group.add_argument(
        "-c",
        "--camera-index",
        type=int,
        help="Camera index (0 for default camera, 1 for second camera, etc.)",
    )

    parser.add_argument("-m", "--model-dir", required=True, help="Directory containing pretrained models")
    parser.add_argument("-s", "--snapshot-dir", help="Directory to save snapshots")

    args = parser.parse_args().__dict__

    processor = VideoProcessor(
        args.get("model_dir"),
        args.get("video_path"),
        args.get("camera_index"),
        args.get("snapshot_dir"),
    )

    processor.run()


if __name__ == "__main__":
    main()
