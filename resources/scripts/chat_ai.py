#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
宠物聊天 AI：调用 Ollama HTTP /api/chat（stream=true）。
stdout：NDJSON 流 + 末尾 done。
ollama_host 优先顺序：环境变量 OLLAMA_HOST > 请求 JSON 中 ollama_host > 默认 http://127.0.0.1:11434
"""

import argparse
import io
import json
import os
import sys
import time
import urllib.error
import urllib.request

DEFAULT_MODEL = "llama3.1:8b"
MAX_TOKENS = 150
TEMPERATURE = 0.7
MAX_INPUT_LENGTH = 200
DEFAULT_OLLAMA_HOST = "http://127.0.0.1:11434"


def resolve_effective_ollama_host(ollama_host_from_json=""):
    env = os.environ.get("OLLAMA_HOST", "").strip()
    if env:
        return env.rstrip("/")
    j = (ollama_host_from_json or "").strip()
    if j:
        return j.rstrip("/")
    return DEFAULT_OLLAMA_HOST


def emit_event(obj):
    sys.stdout.write(json.dumps(obj, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def build_messages(user_input, pet_state, history, conversation_summary):
    """history: list[{"role":"user"|"assistant","content":"..."}]，不含本轮 user_input。"""
    base_rules = """你是一只可爱的虚拟宠物，正在和主人聊天。

请根据以下规则来生成回答：
1. 回答要符合可爱宠物的设定，语气亲切、活泼、简短（不超过100字）
2. 回答必须与用户的问题或话题相关，不要总是说困了或饿了
3. 必须考虑宠物的当前状态，如果宠物饥饿、疲劳或心情不好，回答要体现出来
4. 回答不能包含任何敏感内容
5. 回答要使用中文，保持口语化"""
    parts = [base_rules, "宠物当前状态：" + (pet_state or "（未知）")]
    cs = (conversation_summary or "").strip()
    if cs:
        parts.append("以下为更早对话的摘要（压缩记忆，可能不完整）：\n" + cs)
    system_prompt = "\n\n".join(parts)

    messages = [{"role": "system", "content": system_prompt}]
    if isinstance(history, list):
        for item in history:
            if not isinstance(item, dict):
                continue
            role = item.get("role")
            content = (item.get("content") or "").strip()
            if role not in ("user", "assistant") or not content:
                continue
            messages.append({"role": role, "content": content})
    messages.append({"role": "user", "content": user_input})
    return messages


def get_mock_reply(user_input, pet_state):
    mock_responses = [
        "你好呀！今天心情怎么样？",
        "和你聊天真开心！",
        "可以陪我玩一会儿吗？",
        "今天天气真好呢！",
        "你在忙什么呢？",
        "和你在一起的时光总是很愉快！",
        "今天有没有什么有趣的事发生？",
    ]
    if "饥饿" in pet_state:
        return "我现在好饿呀，能给我点吃的吗？"
    if "疲劳" in pet_state:
        return "我有点累了，想休息一下..."
    if "心情不太好" in pet_state:
        return "我现在心情有点低落..."
    idx = hash(user_input) % len(mock_responses)
    return mock_responses[idx]


def chat_once_collect(messages, model, host_base, num_predict=512, temperature=0.2, timeout_sec=120):
    """非流式 /api/chat，返回 assistant 文本。"""
    url = host_base.rstrip("/") + "/api/chat"
    payload = json.dumps(
        {
            "model": model,
            "messages": messages,
            "stream": False,
            "options": {
                "temperature": temperature,
                "num_predict": num_predict,
            },
        }
    ).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    fp = urllib.request.urlopen(req, timeout=timeout_sec)
    try:
        raw = fp.read()
    finally:
        fp.close()
    data = json.loads(raw.decode("utf-8", errors="replace"))
    err = data.get("error")
    if err:
        raise RuntimeError(str(err))
    msg = data.get("message") or {}
    return (msg.get("content") or "").strip()


def summarize_merge(existing_summary, dialogue_chunk, max_chars, model, host_base):
    sys_prompt = (
        "你是中文对话摘要助手。把「已有摘要」与「新对话片段」合并成一份连贯摘要，"
        "用于后续多轮对话检索。"
    )
    user_prompt = (
        "已有摘要（可为空）：\n%s\n\n需要并入的新对话：\n%s\n\n"
        "要求：输出不超过%d个字符；保留用户偏好、约定、未完成任务、数字与人名；不要寒暄；不要编造。"
        % ((existing_summary or "").strip(), (dialogue_chunk or "").strip(), max_chars)
    )
    messages = [
        {"role": "system", "content": sys_prompt},
        {"role": "user", "content": user_prompt},
    ]
    predict = min(max_chars + 160, 600)
    text = chat_once_collect(messages, model, host_base, num_predict=predict, temperature=0.15)
    if len(text) > max_chars:
        text = text[:max_chars]
    return text


def stream_chat_once(messages, model, max_tokens, temperature, host_base, timeout_sec=180):
    url = host_base.rstrip("/") + "/api/chat"
    payload = json.dumps(
        {
            "model": model,
            "messages": messages,
            "stream": True,
            "options": {
                "temperature": temperature,
                "num_predict": max_tokens,
            },
        }
    ).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    fp = urllib.request.urlopen(req, timeout=timeout_sec)
    try:
        while True:
            raw = fp.readline()
            if not raw:
                break
            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            try:
                data = json.loads(line)
            except ValueError:
                sys.stderr.write("[WARN] 无法解析的一行 NDJSON\n")
                continue
            err = data.get("error")
            if err:
                raise RuntimeError(str(err))
            msg = data.get("message") or {}
            piece = msg.get("content") or ""
            if piece:
                emit_event({"event": "delta", "content": piece})
            if data.get("done"):
                break
    finally:
        fp.close()


def log_http_error_response(code, reason, body):
    snippet = body if body else "(empty body)"
    if len(snippet) > 8192:
        snippet = snippet[:8192] + "\n...[truncated]"
    sys.stderr.write(
        "[ERROR] HTTPError %s %s — response body:\n%s\n"
        % (code, reason or "", snippet)
    )


def run_streaming_chat(messages, user_input, pet_state, model, max_tokens, temperature, host_base):
    last_error = ""
    max_attempts = 3
    hb = host_base.rstrip("/")

    for attempt in range(1, max_attempts + 1):
        should_retry = False
        try:
            stream_chat_once(messages, model, max_tokens, temperature, hb)
            emit_event({"event": "done", "success": True})
            return
        except urllib.error.HTTPError as e:
            try:
                raw = e.read()
                body = raw.decode("utf-8", errors="replace") if raw else ""
            except Exception:
                body = ""
            reason = getattr(e, "reason", "") or ""
            log_http_error_response(e.code, reason, body)
            last_error = (
                "HTTP %s %s — %s"
                % (e.code, reason.strip(), (body.replace("\n", " ")[:500] if body else "(no body)"))
            )
            # 只对服务端/网关错误重试；4xx（含 401）不重试。
            should_retry = e.code >= 500
            more = "，稍后重试" if should_retry and attempt < max_attempts else ""
            sys.stderr.write("[ERROR] AI调用失败（第%d/%d）%s\n" % (attempt, max_attempts, more))
        except urllib.error.URLError as e:
            err = getattr(e, "reason", None) or e
            last_error = str(err)
            sys.stderr.write("[ERROR] URLError(第%d/%d): %s\n" % (attempt, max_attempts, last_error))
            should_retry = True
        except (TimeoutError, ConnectionError, OSError) as e:
            last_error = str(e)
            sys.stderr.write("[ERROR] 网络异常(第%d/%d): %s\n" % (attempt, max_attempts, last_error))
            should_retry = True
        except Exception as e:
            last_error = str(e)
            sys.stderr.write("[ERROR] AI调用异常(第%d/%d): %s\n" % (attempt, max_attempts, last_error))
            should_retry = False

        if should_retry and attempt < max_attempts:
            time.sleep(0.8)
            continue
        break

    reply = get_mock_reply(user_input, pet_state)
    emit_event(
        {
            "event": "done",
            "success": False,
            "reply": reply,
            "error": last_error,
        }
    )


def main():
    parser = argparse.ArgumentParser(description="宠物聊天AI（流式）")
    input_group = parser.add_mutually_exclusive_group()
    input_group.add_argument("--json-input", type=str, help="JSON输入文件路径")
    input_group.add_argument(
        "--stdin-json",
        action="store_true",
        help="从标准输入读取整块 JSON（与 Qt 写入 stdin 配合，避免磁盘临时文件）",
    )
    parser.add_argument("--input", type=str, help="用户输入")
    parser.add_argument("--model", type=str, default=DEFAULT_MODEL)
    parser.add_argument("--max-tokens", type=int, default=MAX_TOKENS)
    parser.add_argument("--temperature", type=float, default=TEMPERATURE)
    parser.add_argument("--pet-state", type=str, default="")

    args = parser.parse_args()
    json_read_error = ""
    ollama_hint_from_request = ""
    history = []
    conversation_summary = ""
    chat_mode = ""
    summarize_existing = ""
    summarize_chunk = ""
    summarize_max_chars = 400

    if args.json_input:
        try:
            with io.open(args.json_input, "r", encoding="utf-8") as f:
                data = json.load(f)
            user_input = data.get("input", "")
            model = data.get("model", DEFAULT_MODEL)
            max_tokens = data.get("max_tokens", MAX_TOKENS)
            temperature = data.get("temperature", TEMPERATURE)
            pet_state = data.get("pet_state", "")
            ollama_hint_from_request = data.get("ollama_host") or ""
            history = data.get("history") or []
            if not isinstance(history, list):
                history = []
            conversation_summary = data.get("conversation_summary") or ""
            chat_mode = (data.get("mode") or "").strip().lower()
            summarize_existing = data.get("existing_summary") or ""
            summarize_chunk = data.get("dialogue_chunk") or ""
            summarize_max_chars = int(data.get("summary_max_chars", 400))
        except Exception as e:
            json_read_error = "[ERROR] 读取JSON文件失败: " + str(e)
            sys.stderr.write(json_read_error + "\n")
            user_input = args.input if args.input else ""
            model = DEFAULT_MODEL
            max_tokens = MAX_TOKENS
            temperature = TEMPERATURE
            pet_state = ""
    elif args.stdin_json:
        try:
            raw = sys.stdin.read()
            if not raw.strip():
                raise ValueError("stdin 为空")
            data = json.loads(raw.strip())
            user_input = data.get("input", "")
            model = data.get("model", DEFAULT_MODEL)
            max_tokens = data.get("max_tokens", MAX_TOKENS)
            temperature = data.get("temperature", TEMPERATURE)
            pet_state = data.get("pet_state", "")
            ollama_hint_from_request = data.get("ollama_host") or ""
            history = data.get("history") or []
            if not isinstance(history, list):
                history = []
            conversation_summary = data.get("conversation_summary") or ""
            chat_mode = (data.get("mode") or "").strip().lower()
            summarize_existing = data.get("existing_summary") or ""
            summarize_chunk = data.get("dialogue_chunk") or ""
            summarize_max_chars = int(data.get("summary_max_chars", 400))
        except Exception as e:
            json_read_error = "[ERROR] 读取 stdin JSON 失败: " + str(e)
            sys.stderr.write(json_read_error + "\n")
            user_input = args.input if args.input else ""
            model = DEFAULT_MODEL
            max_tokens = MAX_TOKENS
            temperature = TEMPERATURE
            pet_state = ""
            ollama_hint_from_request = ""
    else:
        user_input = args.input if args.input else ""
        model = args.model
        max_tokens = args.max_tokens
        temperature = args.temperature
        pet_state = args.pet_state

    user_input = user_input[:MAX_INPUT_LENGTH]

    if json_read_error:
        emit_event(
            {
                "event": "done",
                "success": False,
                "reply": "聊天输入读取失败，请检查 JSON 格式或 stdin 内容。",
                "error": json_read_error,
                "input": user_input,
                "model": model,
            }
        )
        return

    effective_host = resolve_effective_ollama_host(ollama_hint_from_request)

    if chat_mode == "summarize":
        try:
            summary_text = summarize_merge(
                summarize_existing, summarize_chunk, summarize_max_chars, model, effective_host
            )
            emit_event({"event": "done", "success": True, "summary": summary_text})
        except Exception as e:
            emit_event({"event": "done", "success": False, "error": str(e)})
        return

    messages = build_messages(user_input, pet_state, history, conversation_summary)
    run_streaming_chat(
        messages, user_input, pet_state, model, max_tokens, temperature, effective_host
    )


if __name__ == "__main__":
    main()
