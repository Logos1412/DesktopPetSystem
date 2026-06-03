#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GPT-SoVITS 本地 TTS：stdin 一行 JSON，stdout 一行 JSON。
成功: {"ok": true, "wav_path": "..."}
失败: {"ok": false, "error": "..."}
"""

import json
import os
import re
import sys
import tempfile
import urllib.error
import urllib.request

DEFAULT_BASE = "http://127.0.0.1:9874"


def emit(obj):
    sys.stdout.write(json.dumps(obj, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def strip_parentheses(text):
    if not text:
        return ""
    t = text
    t = re.sub(r"（[^）]*）", "", t)
    t = re.sub(r"\([^)]*\)", "", t)
    return re.sub(r"\s+", " ", t).strip()


def synthesize(req):
    base = (req.get("base_url") or DEFAULT_BASE).strip().rstrip("/")
    text = (req.get("text") or "").strip()
    if req.get("strip_parentheses", True):
        text = strip_parentheses(text)
    max_chars = int(req.get("max_chars") or 300)
    if max_chars > 0 and len(text) > max_chars:
        text = text[:max_chars]

    if not text:
        return {"ok": False, "error": "合成文本为空"}

    ref_path = (req.get("ref_audio_path") or "").strip()
    if not ref_path or not os.path.isfile(ref_path):
        return {"ok": False, "error": f"参考音频不存在: {ref_path}"}

    prompt_text = (req.get("prompt_text") or "").strip()
    if not prompt_text:
        return {"ok": False, "error": "prompt_text 为空"}

    body = {
        "text": text,
        "text_lang": (req.get("text_lang") or "zh").strip() or "zh",
        "ref_audio_path": os.path.abspath(ref_path),
        "prompt_text": prompt_text,
        "prompt_lang": (req.get("prompt_lang") or "zh").strip() or "zh",
    }

    url = f"{base}/tts"
    data = json.dumps(body, ensure_ascii=False).encode("utf-8")
    request = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )

    try:
        with urllib.request.urlopen(request, timeout=300) as resp:
            audio = resp.read()
    except urllib.error.HTTPError as e:
        detail = e.read().decode("utf-8", errors="replace")[:500]
        hint = ""
        if e.code == 404:
            hint = (
                "（该地址可能是 WebUI 而非 api_v2：请单独运行 "
                "python api_v2.py -a 127.0.0.1 -p 9880，并把 base_url 改为 http://127.0.0.1:9880）"
            )
        return {"ok": False, "error": f"HTTP {e.code}: {detail}{hint}"}
    except urllib.error.URLError as e:
        return {"ok": False, "error": f"无法连接 TTS 服务 {url}: {e.reason}"}

    if not audio:
        return {"ok": False, "error": "TTS 返回空音频"}

    fd, wav_path = tempfile.mkstemp(prefix="pet_tts_", suffix=".wav")
    os.close(fd)
    with open(wav_path, "wb") as f:
        f.write(audio)
    return {"ok": True, "wav_path": wav_path}


def main():
    raw = sys.stdin.read()
    if not raw.strip():
        emit({"ok": False, "error": "stdin 为空"})
        return 1
    try:
        req = json.loads(raw)
    except json.JSONDecodeError as e:
        emit({"ok": False, "error": f"JSON 无效: {e}"})
        return 1
    emit(synthesize(req))
    return 0


if __name__ == "__main__":
    sys.exit(main())
