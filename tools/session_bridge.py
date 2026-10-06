#!/usr/bin/env python3
"""USB -> local SQLite -> OpenSit 1.0; optional POST to a user-defined endpoint."""
import argparse
import datetime as dt
import json
import os
from pathlib import Path
import sqlite3
import time
import urllib.parse
import urllib.request
import uuid

UTC = dt.timezone.utc
NAMESPACE = uuid.UUID('b67c2821-c20d-4af0-9ed0-91c247b20526')


def session_id(device, sequence):
    return str(uuid.uuid5(NAMESPACE, f'zentimer:{device}:{sequence}'))


def iso(value):
    return dt.datetime.fromtimestamp(value, UTC).isoformat(timespec='milliseconds').replace('+00:00', 'Z')


def parse_date(text):
    value = dt.datetime.fromisoformat(text.replace('Z', '+00:00'))
    if value.utcoffset() is None:
        raise ValueError('Datum braucht Zeitzone, z.B. 2026-10-07T18:30:00+02:00')
    return value.timestamp()


def read_export(lines):
    info, records, active, complete = None, [], False, False
    for raw in lines:
        line = raw.decode('utf-8', errors='replace').strip() if isinstance(raw, bytes) else raw.strip()
        if line.startswith('ZT_INFO '):
            info = json.loads(line[8:])
        elif line == 'ZT_BEGIN':
            if active:
                raise ValueError('Mehrfacher Exportbeginn')
            active = True
        elif line.startswith('ZT_RECORD ') and active:
            records.append(json.loads(line[10:]))
        elif line == 'ZT_END' and active:
            complete = True
            break
    if not info or not complete:
        raise ValueError('Kein vollstaendiger ZT_BEGIN/ZT_END-Export; nichts importiert')
    if not info.get('device'):
        raise ValueError('Geraete-ID fehlt')
    return info, records


def collect(port):
    try:
        import serial
    except ImportError as error:
        raise RuntimeError('USB braucht pyserial: .venv/bin/python -m pip install pyserial') from error
    with serial.Serial(port, 115200, timeout=0.5, write_timeout=5) as connection:
        time.sleep(1)  # USB CDC connection, no hardware reset is requested
        connection.reset_input_buffer()
        connection.write(f'time {int(time.time())}\nexport\n'.encode('ascii'))
        deadline = time.monotonic() + 90
        def lines():
            while time.monotonic() < deadline:
                line = connection.readline()
                if line:
                    yield line
        return read_export(lines())


def database(path):
    connection = sqlite3.connect(path)
    connection.execute('CREATE TABLE IF NOT EXISTS sessions (id TEXT PRIMARY KEY, device TEXT NOT NULL, raw TEXT NOT NULL, started REAL, date_origin TEXT)')
    return connection


def ingest(connection, info, records):
    if info.get('storage') not in ('Bereit', 'Voll (nur Lesen)'):
        raise ValueError(f'Speicher meldet: {info.get("storage")}; kein Import')
    with connection:
        for record in records:
            if record['outcome'] not in ('completed', 'aborted', 'interrupted'):
                raise ValueError('Unbekannter Sitzungsstatus')
            if not (0 <= record['elapsed_ms'] <= record['planned_seconds']*1000):
                raise ValueError('Ungueltige Dauer')
            identifier = session_id(info['device'], record['id'])
            started = record['started_utc'] or None
            origin = 'device' if started else None
            if started is None and record['boot'] == info['boot'] and info.get('utc'):
                delta = info['uptime_ms'] - record['start_uptime_ms']
                if delta >= 0:
                    started = info['utc'] - delta/1000
                    origin = 'same-boot-usb-clock'
            connection.execute('''INSERT INTO sessions VALUES (?, ?, ?, ?, ?)
                ON CONFLICT(id) DO UPDATE SET raw=excluded.raw,
                started=COALESCE(sessions.started,excluded.started),
                date_origin=COALESCE(sessions.date_origin,excluded.date_origin)''',
                (identifier, info['device'], json.dumps(record, sort_keys=True), started, origin))


def make_export(connection, include_interrupted=False):
    sessions, pending = [], []
    for identifier, device, raw, started, origin in connection.execute('SELECT * FROM sessions ORDER BY started,id'):
        record = json.loads(raw)
        reason = 'Datum offen' if started is None else 'Dauer unter einer Sekunde' if record['elapsed_ms'] < 1000 else 'Stromverlust: Dauer nur bis Checkpoint' if record['outcome'] == 'interrupted' and not include_interrupted else None
        if reason:
            pending.append({'id': identifier, 'device': device, 'reason': reason, 'record': record})
            continue
        sessions.append({
            'id': identifier, 'started_at': iso(started), 'duration_seconds': record['elapsed_ms']/1000,
            'source': {'name': 'ZenTimer', 'id': f'{device}:{record["id"]}'},
            'extra': {'zentimer': {'planned_seconds': record['planned_seconds'], 'outcome': record['outcome'], 'date_origin': origin}},
        })
    return {'opensit': '1.0', 'exported_at': iso(time.time()), 'generator': {'name': 'ZenTimer USB Bridge', 'version': '1.0'}, 'sessions': sessions}, pending


def upload(url, payload):
    parsed = urllib.parse.urlparse(url)
    if parsed.scheme != 'https' and not (parsed.scheme == 'http' and parsed.hostname in ('localhost', '127.0.0.1', '::1')):
        raise ValueError('Upload braucht HTTPS (HTTP nur fuer lokalen Test)')
    headers = {'Content-Type': 'application/json'}
    token = os.environ.get('ZENTIMER_UPLOAD_TOKEN')
    if token:
        headers['Authorization'] = f'Bearer {token}'
    request = urllib.request.Request(url, data=json.dumps(payload).encode(), headers=headers, method='POST')
    class NoRedirect(urllib.request.HTTPRedirectHandler):
        def redirect_request(self, request, fp, code, msg, headers, newurl):
            return None  # do not forward personal data or tokens to a redirected endpoint
    with urllib.request.build_opener(NoRedirect).open(request, timeout=30) as response:
        print(f'Upload: HTTP {response.status}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group()
    source.add_argument('--port', help='USB-Port, Monitor vorher schliessen')
    source.add_argument('--input', type=Path, help='Gespeichertes Serial-Exportprotokoll statt USB')
    parser.add_argument('--output-dir', type=Path, default=Path('data/sessions'))
    parser.add_argument('--resolve-date', action='append', default=[], metavar='UUID=RFC3339', help='Datum einer aelteren Sitzung manuell zuordnen')
    parser.add_argument('--include-interrupted', action='store_true', help='Unvollstaendige Checkpoint-Dauern bewusst mit exportieren')
    parser.add_argument('--upload-url', help='Eigener POST-Endpunkt, keine Timefully-API')
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    with database(args.output_dir/'sessions.sqlite3') as connection:
        if args.port or args.input:
            info, records = collect(args.port) if args.port else read_export(args.input.read_text().splitlines())
            ingest(connection, info, records)
        for assignment in args.resolve_date:
            identifier, value = assignment.split('=', 1)
            uuid.UUID(identifier)
            with connection:
                if connection.execute('UPDATE sessions SET started=?,date_origin=? WHERE id=?', (parse_date(value), 'manual', identifier)).rowcount != 1:
                    raise ValueError(f'Sitzung {identifier} fehlt in der lokalen Ablage')
        payload, pending = make_export(connection, args.include_interrupted)
        for name, content in (('sessions.opensit.json', payload), ('pending.json', pending)):
            target = args.output_dir/name
            temporary = target.with_suffix(target.suffix+'.tmp')
            temporary.write_text(json.dumps(content, ensure_ascii=False, indent=2)+'\n')
            temporary.replace(target)
        print(f'{len(payload["sessions"])} exportierbar, {len(pending)} offen. Ablage: {args.output_dir.resolve()}')
        if args.upload_url:
            upload(args.upload_url, payload)


if __name__ == '__main__':
    try:
        main()
    except (ValueError, RuntimeError, OSError, KeyError, sqlite3.Error) as error:
        raise SystemExit(str(error))
