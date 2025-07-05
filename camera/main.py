#!.venv/bin/python3

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

import argparse
import sys

from video_processor import VideoProcessor


def main() -> int:
    parser = argparse.ArgumentParser(description="Video Processor")
    parser.epilog = f"Example: {parser.prog} -c 0 --model-dir build/downloads --snapshot-dir build/snapshots"

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
    return 0


if __name__ == "__main__":
    sys.exit(main())
