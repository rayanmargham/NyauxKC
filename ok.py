#!/usr/bin/env python3
import sys, socket, json, time

# Mapping of characters to QEMU qcode names
PUNCT_MAP = {
    ' ': 'spc',           # QEMU code for space
    ',': 'comma',
    '.': 'dot',
    '!': 'exclam',
    '?': 'question',
    '-': 'minus',
    '_': 'underscore',
    ':': 'colon',
    ';': 'semicolon',
    '/': 'slash',
    '\\': 'backslash',
    "'": 'apostrophe',
    '"': 'quote',
    '@': 'at',
    '#': 'numbersign',
    '$': 'dollar',
    '%': 'percent',
    '^': 'caret',
    '&': 'ampersand',
    '(': 'leftparen',
    ')': 'rightparen',
    '+': 'plus',
    '=': 'equal',
    '<': 'less',
    '>': 'greater',
    '[': 'bracket_left',
    ']': 'bracket_right',
    '`': 'grave_accent',
    '~': 'tilde',
    '\t': 'tab',
    '\b': 'backspace',
}
ENTER_QCODE = 'ret'  # QEMU code for main Enter/Return key
USE_SENDKEY = False   # Prefer input-send-event for reliable text input


def qmp_connect(sock_path):
    """Connect to QMP socket and negotiate capabilities."""
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(sock_path)
    sock.recv(4096)  # greeting
    sock.sendall((json.dumps({"execute": "qmp_capabilities"}) + "\n").encode())
    sock.recv(4096)
    return sock


def qmp_input_event(sock, events):
    """Send low-level input events using input-send-event."""
    cmd = {"execute": "input-send-event", "arguments": {"events": events}}
    sock.sendall((json.dumps(cmd) + "\n").encode())
    return sock.recv(4096)


def build_events_for_char(ch):
    """Build a sequence of input events for a single character."""
    events = []
    # Newline -> Enter
    if ch == '\n':
        events.append({"type": "key", "data": {"key": {"type": "qcode", "data": ENTER_QCODE}, "down": True}})
        events.append({"type": "key", "data": {"key": {"type": "qcode", "data": ENTER_QCODE}, "down": False}})
        return events

    # Alphabetic characters
    if ch.isalpha():
        base = ch.lower()
        if ch.isupper():  # uppercase: Shift + letter
            events.append({"type": "key", "data": {"key": {"type": "qcode", "data": "shift"}, "down": True}})
            events.append({"type": "key", "data": {"key": {"type": "qcode", "data": base}, "down": True}})
            events.append({"type": "key", "data": {"key": {"type": "qcode", "data": base}, "down": False}})
            events.append({"type": "key", "data": {"key": {"type": "qcode", "data": "shift"}, "down": False}})
        else:  # lowercase
            events.append({"type": "key", "data": {"key": {"type": "qcode", "data": base}, "down": True}})
            events.append({"type": "key", "data": {"key": {"type": "qcode", "data": base}, "down": False}})
        return events

    # Digits
    if ch.isdigit():
        events.append({"type": "key", "data": {"key": {"type": "qcode", "data": ch}, "down": True}})
        events.append({"type": "key", "data": {"key": {"type": "qcode", "data": ch}, "down": False}})
        return events

    # Punctuation / other mapped chars
    code = PUNCT_MAP.get(ch)
    if code:
        events.append({"type": "key", "data": {"key": {"type": "qcode", "data": code}, "down": True}})
        events.append({"type": "key", "data": {"key": {"type": "qcode", "data": code}, "down": False}})
        return events

    # Fallback: unknown char
    print(f"Warning: no mapping for character {repr(ch)}, skipping.", file=sys.stderr)
    return events


def main():
    if len(sys.argv) < 3:
        print("Usage: qmp_type_loop.py <qmp-socket-path> <text-to-send> [interval-seconds]", file=sys.stderr)
        sys.exit(1)

    sock_path = sys.argv[1]
    raw_text = sys.argv[2]
    interval = float(sys.argv[3]) if len(sys.argv) >= 4 else 1.0

    # Replace literal escapes
    text = raw_text.replace('\\n', '\n').replace('\\t', '\t')

    sock = qmp_connect(sock_path)
    try:
        while True:
            for ch in text:
                events = build_events_for_char(ch)
                if events:
                    qmp_input_event(sock, events)
            time.sleep(interval)
    except KeyboardInterrupt:
        print("Interrupted by user, closing connection.")
    finally:
        sock.close()

if __name__ == "__main__":
    main()

