#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Convert request TXT files from Git history to JavaScript format
"""

import subprocess
import json
import re
import os
from datetime import datetime

# Configuration
REPO_PATH = r"d:\04.pretests-iot\251020_pnklabmem203_esp32p4_switchbox_PoC"
OUTPUT_DIR = r"d:\04.pretests-iot\251020_pnklabmem203_esp32p4_switchbox_PoC\Ai\request"
GIT_COMMIT = "1120ad1^"  # Parent of commit that removed txt files

def run_git_command(args):
    """Run git command and return output"""
    cmd = ["git"] + args
    result = subprocess.run(
        cmd,
        cwd=REPO_PATH,
        capture_output=True,
        text=True,
        encoding='utf-8'
    )
    if result.returncode != 0:
        print(f"Git command failed: {' '.join(cmd)}")
        print(f"Error: {result.stderr}")
        return None
    return result.stdout

def read_txt_from_git(request_num):
    """Read request TXT file from Git history"""
    path = f"251020_pnklabmem203_esp32p4_switchbox_PoC/Ai/request/request({request_num}).txt"
    content = run_git_command(["show", f"{GIT_COMMIT}:{path}"])
    return content

def parse_txt_content(content, request_num):
    """Parse TXT content and extract structured data"""
    if not content:
        return None

    # Extract title (first line with request number)
    title_match = re.search(r'요청 #\d+\s*-\s*(.+)', content)
    title = title_match.group(1).strip() if title_match else f"Request {request_num}"

    # Extract date
    date_match = re.search(r'날짜:\s*(\d{4}-\d{2}-\d{2})', content)
    date = date_match.group(1) if date_match else "2025-10-20"

    # Split by question/answer sections
    sections = {}

    # Extract question
    question_match = re.search(r'\[질문\](.*?)(?:\[답변\]|\[질문 #\d+\]|$)', content, re.DOTALL)
    if question_match:
        question_text = question_match.group(1).strip()
        question_text = re.sub(r'^━+$', '', question_text, flags=re.MULTILINE).strip()
        sections['question'] = question_text

    # Extract answer
    answer_match = re.search(r'\[답변\].*?\n(.*?)(?:\[질문 #\d+\]|═+$|$)', content, re.DOTALL)
    if answer_match:
        answer_text = answer_match.group(1).strip()
        answer_text = re.sub(r'^━+$', '', answer_text, flags=re.MULTILINE).strip()
        sections['answer'] = answer_text

    # Extract subsections from answer (문제, 해결, 등)
    subsections = {}
    subsection_patterns = [
        (r'■\s*(.+?)\n(.*?)(?=■|$)', 'section'),
        (r'【(.+?)】\n(.*?)(?=【|■|$)', 'subsection'),
    ]

    answer_text = sections.get('answer', '')
    for pattern, section_type in subsection_patterns:
        for match in re.finditer(pattern, answer_text, re.DOTALL):
            section_title = match.group(1).strip()
            section_content = match.group(2).strip()
            subsections[section_title] = section_content

    # Generate tags based on content
    tags = []
    content_lower = content.lower()

    tag_keywords = {
        'camera': ['카메라', 'camera', 'mipi', 'csi'],
        'h264': ['h.264', 'h264', '인코더', 'encoder'],
        'sd_card': ['sd카드', 'sd card', 'sdmmc'],
        'jpeg': ['jpeg', 'jpg', 'mjpeg'],
        'video': ['동영상', 'video', '녹화', 'recording'],
        'debug': ['디버그', 'debug', '로그', 'log'],
        'memory': ['메모리', 'memory', 'buffer', '버퍼'],
        'error': ['에러', 'error', '문제', 'issue', 'bug'],
    }

    for tag, keywords in tag_keywords.items():
        if any(keyword in content_lower for keyword in keywords):
            tags.append(tag)

    if not tags:
        tags = ['general']

    # Build data structure
    data = {
        "request_number": request_num,
        "title": title,
        "date": date,
        "author": "Claude (AI Assistant)",
        "status": "completed",
        "tags": tags,
        "raw_content": {
            "question": sections.get('question', ''),
            "answer": sections.get('answer', '')
        },
        "sections": subsections if subsections else sections
    }

    return data

def generate_js_file(data, request_num):
    """Generate JavaScript file from data"""
    js_content = f"""// request({request_num}).js
window.requestData = window.requestData || [];
window.requestData[{request_num}] = {json.dumps(data, ensure_ascii=False, indent=2)};
"""

    output_file = os.path.join(OUTPUT_DIR, f"request({request_num}).js")
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(js_content)

    print(f"✓ Generated: request({request_num}).js")
    return output_file

def main():
    """Main function to process all request files"""
    print("=" * 70)
    print("Converting Request TXT files to JavaScript")
    print("=" * 70)
    print()

    # Process requests 1-35
    request_numbers = list(range(1, 36))

    success_count = 0
    error_count = 0

    for num in request_numbers:
        try:
            print(f"Processing request({num})...", end=" ")

            # Read TXT from Git
            txt_content = read_txt_from_git(num)

            if not txt_content:
                print(f"✗ Not found in Git history")
                error_count += 1
                continue

            # Parse content
            data = parse_txt_content(txt_content, num)

            if not data:
                print(f"✗ Failed to parse")
                error_count += 1
                continue

            # Generate JS file
            generate_js_file(data, num)
            success_count += 1

        except Exception as e:
            print(f"✗ Error: {e}")
            error_count += 1

    print()
    print("=" * 70)
    print(f"Conversion complete!")
    print(f"Success: {success_count} files")
    print(f"Errors: {error_count} files")
    print("=" * 70)

if __name__ == "__main__":
    main()
