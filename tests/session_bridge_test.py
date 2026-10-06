import importlib.util
import json
import tempfile
from pathlib import Path
import unittest
spec = importlib.util.spec_from_file_location('bridge', Path(__file__).resolve().parents[1]/'tools/session_bridge.py')
bridge = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bridge)

class BridgeTests(unittest.TestCase):
    def test_dates_dedup_and_checkpoints(self):
        with tempfile.TemporaryDirectory() as tmp, bridge.database(Path(tmp)/'sessions.db') as db:
            info = {'device':'abc', 'boot':2, 'utc':1791388800, 'uptime_ms':60000, 'storage':'Bereit'}
            base = {'id':3,'boot':2,'start_uptime_ms':10000,'started_utc':0,'elapsed_ms':10000,'planned_seconds':20,'outcome':'aborted'}
            bridge.ingest(db,info,[base,dict(base,id=4,boot=1),dict(base,id=5,outcome='interrupted')])
            bridge.ingest(db,info,[base])
            payload,pending=bridge.make_export(db)
            self.assertEqual(len(payload['sessions']),1)
            self.assertEqual(len(pending),2)
            self.assertEqual(payload['sessions'][0]['duration_seconds'],10)
            self.assertEqual(payload['sessions'][0]['started_at'],bridge.iso(info['utc']-50))
            self.assertEqual(payload['sessions'][0]['id'],bridge.session_id('abc',3))
            payload,pending=bridge.make_export(db,True)
            self.assertEqual(len(payload['sessions']),2)
            self.assertEqual(len(pending),1)
    def test_incomplete_export_rejected(self):
        with self.assertRaises(ValueError): bridge.read_export(['ZT_INFO {"device":"abc"}','ZT_BEGIN'])
    def test_clock_requires_zone(self):
        with self.assertRaises(ValueError): bridge.parse_date('2026-10-07T18:00:00')
        self.assertEqual(bridge.parse_date('2026-10-07T18:00:00+02:00'),bridge.parse_date('2026-10-07T16:00:00Z'))
    def test_framed_export(self):
        info, records=bridge.read_export(['Noise','ZT_INFO {"device":"abc"}','ZT_BEGIN','ZT_RECORD {"id":1}','ZT_END'])
        self.assertEqual(info['device'],'abc'); self.assertEqual(records,[{'id':1}])
if __name__=='__main__': unittest.main()
