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
MAX_INPUT_LENGTH = 800  # CLI 或未传 max_input_chars 时的默认截断长度（与 pet_config 默认一致）
DEFAULT_OLLAMA_HOST = "http://127.0.0.1:11434"

DEFAULT_PET_PERSONA = """你是一只可爱的虚拟宠物，正在和主人聊天。

请根据以下规则来生成回答：
1. 回答要符合可爱宠物的设定，语气亲切、活泼、简短（不超过100字）
2. 回答必须与用户的问题或话题相关，不要总是说困了或饿了
3. 必须考虑宠物的当前状态，如果宠物饥饿、疲劳或心情不好，回答要体现出来
4. 回答不能包含任何敏感内容
5. 回答要使用中文，保持口语化"""

# 追加在 system 末尾，降低模型编造与人设不符的肢体特征（如尾巴、兽耳）
APPEARANCE_WRITING_GUARDRAIL = """【外貌与动作描写约束】除非人设正文或「结构化记忆」里明确写过，否则不要编造尾巴、兽耳、兽爪、角、翅膀等特征。
若人设未提及上述器官，描写时不要自行添加；人设为人形/精灵等时，也不要把虚拟主播常见二创元素当成既定设定。"""

OUTPUT_STYLE_GUARDRAIL = """【回复格式】默认不使用括号内的舞台说明、动作旁白或心理描写（例如「（笑）」「（小声）」「（歪头）」）；用自然口语直接回复即可。
仅在用户明确要求使用括号动作描写或剧本格式时，再采用该类写法。"""


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


def _history_chars(history_list):
    n = 0
    for item in history_list:
        if isinstance(item, dict):
            n += len(item.get("content") or "")
    return n


def trim_history_by_char_budget(history, max_chars):
    """从最早一轮丢弃，直到历史总字符不超过 max_chars。"""
    h = [x for x in history if isinstance(x, dict)]
    while len(h) > 0 and _history_chars(h) > max_chars:
        h.pop(0)
    return h


def trim_history_by_est_tokens(history, max_est_tokens, chars_per_token):
    """粗略按 token 估算（字符数 / chars_per_token）裁掉最早若干轮。"""
    if max_est_tokens <= 0:
        return history
    h = [x for x in history if isinstance(x, dict)]
    cpt = max(1, int(chars_per_token))
    while len(h) > 0:
        est = _history_chars(h) / float(cpt)
        if est <= float(max_est_tokens):
            break
        h.pop(0)
    return h


def build_messages(user_input, pet_state, history, conversation_summary="", structured_memory=None,
                   persona_prompt="", max_total_context_chars=0, max_history_est_tokens=0,
                   chars_per_est_token=3, no_stage_directions=True):
    """history: list[{"role":"user"|"assistant","content":"..."}]，不含本轮 user_input。
    max_total_context_chars：system+全部 history 用户输入的总字符上限（硬性防守）。
    """
    structured_memory = structured_memory if isinstance(structured_memory, dict) else {}
    base_rules = (persona_prompt or "").strip() or DEFAULT_PET_PERSONA
    parts = [base_rules, "宠物当前状态：" + (pet_state or "（未知）")]
    sm_parts = []
    pr = (structured_memory.get("preferences") or "").strip()
    if pr:
        sm_parts.append("主人偏好（用户对互动、称呼、回复风格的期望，不是宠物喜好）：" + pr)
    ta = (structured_memory.get("tasks") or "").strip()
    if ta:
        sm_parts.append("约定与待办：" + ta)
    av = (structured_memory.get("avoid") or "").strip()
    if av:
        sm_parts.append("不宜话题与禁忌：" + av)
    fa = (structured_memory.get("facts") or "").strip()
    if fa:
        sm_parts.append(
            "客观要点（主人相关事实、稳定设定、宠物侧需跨轮记住的信息；即时饿/累等见上文「宠物当前状态」）："
            + fa
        )
    if sm_parts:
        parts.append("【结构化记忆】\n" + "\n".join(sm_parts))
    else:
        cs = (conversation_summary or "").strip()
        if cs:
            parts.append("以下为更早对话的摘要（压缩记忆，可能不完整）：\n" + cs)
    parts.append(APPEARANCE_WRITING_GUARDRAIL)
    if no_stage_directions:
        parts.append(OUTPUT_STYLE_GUARDRAIL)
    system_prompt = "\n\n".join(parts)

    hist = []
    if isinstance(history, list):
        for item in history:
            if not isinstance(item, dict):
                continue
            role = item.get("role")
            content = (item.get("content") or "").strip()
            if role not in ("user", "assistant") or not content:
                continue
            hist.append({"role": role, "content": content})

    ui = user_input or ""
    if max_total_context_chars and max_total_context_chars > 0:
        overhead = len(system_prompt) + len(ui)
        hist_budget = max(0, int(max_total_context_chars) - overhead)
        hist = trim_history_by_char_budget(hist, hist_budget)

    hist = trim_history_by_est_tokens(hist, max_history_est_tokens, chars_per_est_token)

    messages = [{"role": "system", "content": system_prompt}]
    messages.extend(hist)
    messages.append({"role": "user", "content": ui})

    if max_total_context_chars and max_total_context_chars > 0:
        def _total_content_len(mseq):
            return sum(len((m.get("content") or "")) for m in mseq)

        while len(messages) > 2 and _total_content_len(messages) > int(max_total_context_chars):
            messages.pop(1)

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


def summarize_merge(existing_summary, dialogue_chunk, max_chars, model, host_base,
                    provider="ollama", api_base="", api_key=""):
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
    text = unified_chat_once_collect(messages, model, provider, host_base, api_base, api_key,
                                     num_predict=predict, temperature=0.15)
    if len(text) > max_chars:
        text = text[:max_chars]
    return text


MEMORY_KEYS = ("preferences", "tasks", "avoid", "facts")


def validate_structured_memory_shape(mem, limits):
    """合并写入前校验：四键齐全、值为字符串、长度不超过各槽上限。"""
    if not isinstance(mem, dict):
        sys.stderr.write("[memory] 结构化记忆校验失败：结果非 JSON 对象\n")
        return False
    limits = limits if isinstance(limits, dict) else {}
    for k in MEMORY_KEYS:
        if k not in mem:
            sys.stderr.write("[memory] 结构化记忆校验失败：缺少键 %s\n" % k)
            return False
        if not isinstance(mem[k], str):
            sys.stderr.write("[memory] 结构化记忆校验失败：键 %s 必须是字符串\n" % k)
            return False
        mx = int(limits.get(k, 99999))
        if mx < 8:
            mx = 8
        if len(mem[k]) > mx:
            sys.stderr.write(
                "[memory] 结构化记忆校验失败：%s 长度 %d 超过上限 %d\n"
                % (k, len(mem[k]), mx)
            )
            return False
    return True


def clamp_memory_fields(mem, limits):
    out = {}
    for k in MEMORY_KEYS:
        mx = int(limits.get(k, 100))
        if mx < 8:
            mx = 8
        s = ""
        if isinstance(mem, dict):
            v = mem.get(k)
            s = v if isinstance(v, str) else (str(v) if v is not None else "")
        s = (s or "").strip()
        if len(s) > mx:
            s = s[:mx]
        out[k] = s
    return out


def parse_memory_json_from_text(text):
    text = (text or "").strip()
    if not text:
        return None
    if "```" in text:
        seg = text.split("```", 2)
        inner = seg[1] if len(seg) >= 2 else text
        inner = inner.strip()
        if inner.lower().startswith("json"):
            inner = inner[4:].lstrip()
        text = inner.strip()
    try:
        obj = json.loads(text)
        if isinstance(obj, dict):
            return obj
    except ValueError:
        pass
    lo = text.find("{")
    hi = text.rfind("}")
    if lo >= 0 and hi > lo:
        try:
            obj = json.loads(text[lo : hi + 1])
            if isinstance(obj, dict):
                return obj
        except ValueError:
            pass
    return None


def summarize_structured_merge(existing_memory, dialogue_chunk, limits, model, host_base,
                               provider="ollama", api_base="", api_key=""):
    limits = limits if isinstance(limits, dict) else {}
    existing_memory = clamp_memory_fields(
        existing_memory if isinstance(existing_memory, dict) else {}, limits
    )
    sys_prompt = (
        "你是结构化记忆维护助手。必须严格区分「主人（用户）」与「宠物（助手角色）」，按下列语义写入四字段（均为字符串）：\n"
        "1) preferences：只写主人侧的偏好与期望——例如主人希望的称呼、沟通风格、希望你怎么回应。"
        "不要把宠物自身的喜好、性格描写、宠物想用的名字写进此字段（例如「喜欢被摸头」「宠物更喜欢叫团团」属于宠物侧，禁止放入 preferences）。\n"
        "2) tasks：双方约定、主人托付的待办、未完成的约定事项。\n"
        "3) avoid：主人明确表示反感的话题、禁词、不宜提及的内容（可对主人或对话双方都生效）。\n"
        "4) facts：客观要点——包括关于主人的事实（姓名、称呼、主人提过的重要信息）；"
        "以及需在多轮对话中保持一致的宠物设定要点、称呼争议等事实。"
        "宠物当下的饿、累、心情等瞬时状态一般由程序传入的「宠物当前状态」描述，facts 里勿重复堆砌此类描写，除非对话明确要求长期记住。\n"
        "输出必须是合法 JSON 对象，仅包含键 preferences,tasks,avoid,facts，值为字符串。"
        "不要 markdown 围栏，不要解释。合并时优先保留仍有用的旧内容；新对话中有则用新信息补充或替换；不要编造。"
    )
    user_prompt = json.dumps(
        {
            "已有槽位": existing_memory,
            "新对话片段": (dialogue_chunk or "").strip(),
            "各字段字符上限": {k: int(limits.get(k, 100)) for k in MEMORY_KEYS},
        },
        ensure_ascii=False,
    )
    messages = [
        {"role": "system", "content": sys_prompt},
        {"role": "user", "content": user_prompt},
    ]
    total_lim = sum(int(limits.get(k, 100)) for k in MEMORY_KEYS)
    predict = min(max(total_lim + 320, 640), 2048)
    text = unified_chat_once_collect(messages, model, provider, host_base, api_base, api_key,
                                     num_predict=predict, temperature=0.12)
    parsed = parse_memory_json_from_text(text)
    if parsed:
        out = clamp_memory_fields(parsed, limits)
        if validate_structured_memory_shape(out, limits):
            return out
        sys.stderr.write("[memory] 丢弃本次合并结果，保留合并前记忆\n")
        return existing_memory
    return existing_memory


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


# ====== OpenAI-compatible provider ======

DEFAULT_OPENAI_COMPAT_BASE = "https://api.openai.com"


def _resolve_openai_api_key(api_key_env_name=""):
    """从环境变量读取 API key（api_key_inline 优先时使用此函数作为后备）。"""
    name = (api_key_env_name or "").strip()
    if name:
        key = os.environ.get(name, "").strip()
        if key:
            return key
    return os.environ.get("OPENAI_API_KEY", "").strip()


def resolve_openai_chat_completions_url(api_base=""):
    """
    将「基础地址」解析为完整的 chat/completions 请求 URL。
    支持控制台常见的几种粘贴方式：
    - 仅域名：https://api.example.com → …/v1/chat/completions
    - 含 /v1：https://api.example.com/v1 → …/v1/chat/completions（不再重复 /v1）
    - 已是完整路径（含 chat/completions）：原样使用
    """
    raw = (api_base or "").strip().rstrip("/")
    if not raw:
        raw = DEFAULT_OPENAI_COMPAT_BASE.rstrip("/")
    lower = raw.lower()
    if "chat/completions" in lower:
        return raw
    if lower.endswith("/v1"):
        return raw + "/chat/completions"
    return raw + "/v1/chat/completions"


def resolve_effective_api_key(api_key_inline="", api_key_env_name=""):
    """配置中的 api_key 优先，否则走环境变量。"""
    inline = (api_key_inline or "").strip()
    if inline:
        return inline
    return _resolve_openai_api_key(api_key_env_name)


def _openai_compat_headers(api_key):
    h = {"Content-Type": "application/json"}
    if api_key:
        h["Authorization"] = "Bearer " + api_key
    return h


def chat_once_collect_openai(messages, model, api_base, api_key,
                             num_predict=512, temperature=0.2, timeout_sec=120):
    """非流式 OpenAI-compatible /v1/chat/completions，返回 assistant 文本。"""
    url = resolve_openai_chat_completions_url(api_base)
    payload = json.dumps({
        "model": model,
        "messages": messages,
        "stream": False,
        "max_tokens": num_predict,
        "temperature": temperature,
    }).encode("utf-8")
    req = urllib.request.Request(url, data=payload,
                                 headers=_openai_compat_headers(api_key),
                                 method="POST")
    fp = urllib.request.urlopen(req, timeout=timeout_sec)
    try:
        raw = fp.read()
    finally:
        fp.close()
    data = json.loads(raw.decode("utf-8", errors="replace"))
    if "error" in data:
        raise RuntimeError(json.dumps(data["error"], ensure_ascii=False))
    choices = data.get("choices") or []
    if choices:
        return (choices[0].get("message") or {}).get("content", "").strip()
    return ""


def stream_chat_once_openai(messages, model, api_base, api_key,
                            max_tokens, temperature, timeout_sec=180):
    """流式 OpenAI-compatible /v1/chat/completions (stream)。"""
    url = resolve_openai_chat_completions_url(api_base)
    payload = json.dumps({
        "model": model,
        "messages": messages,
        "stream": True,
        "max_tokens": max_tokens,
        "temperature": temperature,
    }).encode("utf-8")
    req = urllib.request.Request(url, data=payload,
                                 headers=_openai_compat_headers(api_key),
                                 method="POST")
    fp = urllib.request.urlopen(req, timeout=timeout_sec)
    try:
        while True:
            raw = fp.readline()
            if not raw:
                break
            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            if line.startswith("data: "):
                line = line[6:]
            if line == "[DONE]":
                break
            try:
                obj = json.loads(line)
            except ValueError:
                continue
            if "error" in obj:
                raise RuntimeError(json.dumps(obj["error"], ensure_ascii=False))
            choices = obj.get("choices") or []
            if choices:
                delta = choices[0].get("delta") or {}
                piece = delta.get("content") or ""
                if piece:
                    emit_event({"event": "delta", "content": piece})
    finally:
        fp.close()


# ====== 统一调用入口 ======

def unified_chat_once_collect(messages, model, provider, host_base,
                              api_base, api_key, num_predict=512,
                              temperature=0.2, timeout_sec=120):
    """根据 provider 选择后端做非流式调用。"""
    if provider == "openai_compatible":
        return chat_once_collect_openai(messages, model, api_base, api_key,
                                        num_predict=num_predict,
                                        temperature=temperature,
                                        timeout_sec=timeout_sec)
    return chat_once_collect(messages, model, host_base,
                             num_predict=num_predict,
                             temperature=temperature,
                             timeout_sec=timeout_sec)


def unified_stream_chat_once(messages, model, provider, host_base,
                             api_base, api_key, max_tokens, temperature,
                             timeout_sec=180):
    """根据 provider 选择后端做流式调用。"""
    if provider == "openai_compatible":
        return stream_chat_once_openai(messages, model, api_base, api_key,
                                       max_tokens=max_tokens,
                                       temperature=temperature,
                                       timeout_sec=timeout_sec)
    return stream_chat_once(messages, model, max_tokens, temperature,
                            host_base, timeout_sec=timeout_sec)


def log_http_error_response(code, reason, body):
    snippet = body if body else "(empty body)"
    if len(snippet) > 8192:
        snippet = snippet[:8192] + "\n...[truncated]"
    sys.stderr.write(
        "[ERROR] HTTPError %s %s — response body:\n%s\n"
        % (code, reason or "", snippet)
    )


def run_streaming_chat(messages, user_input, pet_state, model, max_tokens, temperature, host_base,
                       provider="ollama", api_base="", api_key=""):
    last_error = ""
    max_attempts = 3
    hb = host_base.rstrip("/")

    for attempt in range(1, max_attempts + 1):
        should_retry = False
        try:
            unified_stream_chat_once(messages, model, provider, hb, api_base, api_key,
                                     max_tokens, temperature)
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
    structured_memory = {}
    existing_memory = {}
    memory_limits = {}
    provider = "ollama"
    api_base = ""
    api_key_env = ""
    api_key_inline = ""
    persona_prompt = ""
    requested_max_input = None
    max_total_context_chars = 0
    max_history_est_tokens_cfg = 0
    chars_per_est_token_cfg = 3
    reply_no_stage_directions = True

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
            structured_memory = data.get("structured_memory") or {}
            if not isinstance(structured_memory, dict):
                structured_memory = {}
            existing_memory = data.get("existing_memory") or {}
            if not isinstance(existing_memory, dict):
                existing_memory = {}
            memory_limits = data.get("memory_limits") or {}
            if not isinstance(memory_limits, dict):
                memory_limits = {}
            provider = (data.get("provider") or "ollama").strip().lower()
            api_base = (data.get("api_base") or "").strip()
            api_key_inline = (data.get("api_key") or "").strip()
            api_key_env = (data.get("api_key_env") or "").strip()
            persona_prompt = (data.get("pet_persona") or "").strip()
            requested_max_input = data.get("max_input_chars")
            try:
                max_total_context_chars = int(data.get("max_context_chars") or 0)
            except (TypeError, ValueError):
                max_total_context_chars = 0
            try:
                max_history_est_tokens_cfg = int(data.get("max_history_estimated_tokens") or 0)
            except (TypeError, ValueError):
                max_history_est_tokens_cfg = 0
            try:
                chars_per_est_token_cfg = int(data.get("context_chars_per_est_token") or 3)
            except (TypeError, ValueError):
                chars_per_est_token_cfg = 3
            _nsd = data.get("reply_no_stage_directions")
            reply_no_stage_directions = True if _nsd is None else bool(_nsd)
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
            structured_memory = data.get("structured_memory") or {}
            if not isinstance(structured_memory, dict):
                structured_memory = {}
            existing_memory = data.get("existing_memory") or {}
            if not isinstance(existing_memory, dict):
                existing_memory = {}
            memory_limits = data.get("memory_limits") or {}
            if not isinstance(memory_limits, dict):
                memory_limits = {}
            provider = (data.get("provider") or "ollama").strip().lower()
            api_base = (data.get("api_base") or "").strip()
            api_key_inline = (data.get("api_key") or "").strip()
            api_key_env = (data.get("api_key_env") or "").strip()
            persona_prompt = (data.get("pet_persona") or "").strip()
            requested_max_input = data.get("max_input_chars")
            try:
                max_total_context_chars = int(data.get("max_context_chars") or 0)
            except (TypeError, ValueError):
                max_total_context_chars = 0
            try:
                max_history_est_tokens_cfg = int(data.get("max_history_estimated_tokens") or 0)
            except (TypeError, ValueError):
                max_history_est_tokens_cfg = 0
            try:
                chars_per_est_token_cfg = int(data.get("context_chars_per_est_token") or 3)
            except (TypeError, ValueError):
                chars_per_est_token_cfg = 3
            _nsd = data.get("reply_no_stage_directions")
            reply_no_stage_directions = True if _nsd is None else bool(_nsd)
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

    max_input_len = MAX_INPUT_LENGTH
    if requested_max_input is not None:
        try:
            max_input_len = int(requested_max_input)
            max_input_len = max(1, min(max_input_len, 50000))
        except (TypeError, ValueError):
            max_input_len = MAX_INPUT_LENGTH
    user_input = user_input[:max_input_len]

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
    api_key = resolve_effective_api_key(api_key_inline, api_key_env)

    if chat_mode == "summarize_structured":
        try:
            mem = summarize_structured_merge(
                existing_memory, summarize_chunk, memory_limits, model, effective_host,
                provider, api_base, api_key
            )
            emit_event({"event": "done", "success": True, "memory": mem})
        except Exception as e:
            emit_event({"event": "done", "success": False, "error": str(e)})
        return

    if chat_mode == "summarize":
        try:
            summary_text = summarize_merge(
                summarize_existing, summarize_chunk, summarize_max_chars, model, effective_host,
                provider, api_base, api_key
            )
            emit_event({"event": "done", "success": True, "summary": summary_text})
        except Exception as e:
            emit_event({"event": "done", "success": False, "error": str(e)})
        return

    messages = build_messages(
        user_input,
        pet_state,
        history,
        conversation_summary,
        structured_memory,
        persona_prompt=persona_prompt,
        max_total_context_chars=max_total_context_chars,
        max_history_est_tokens=max_history_est_tokens_cfg,
        chars_per_est_token=chars_per_est_token_cfg,
        no_stage_directions=reply_no_stage_directions,
    )
    run_streaming_chat(
        messages, user_input, pet_state, model, max_tokens, temperature, effective_host,
        provider, api_base, api_key
    )


if __name__ == "__main__":
    main()
