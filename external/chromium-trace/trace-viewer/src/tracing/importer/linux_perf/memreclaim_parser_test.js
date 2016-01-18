// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

base.require('tracing.test_utils');
base.require('tracing.importer.linux_perf_importer');

base.unittest.testSuite('tracing.importer.linux_perf.memreclaim_parser', function() { // @suppress longLineCheck
  test('memreclaimImport', function() {
    var lines = [
      ' surfaceflinger-1155  ( 1155) [001] ...1 12839.528756: ' +
	         'mm_vmscan_direct_reclaim_begin: order=0 ' +
	         'may_writepage=1 ' +
                 'gfp_flags=GFP_KERNEL|GFP_NOWARN|GFP_ZERO|0x2',
      ' surfaceflinger-1155  ( 1155) [001] ...1 12839.531950: ' +
                 'mm_vmscan_direct_reclaim_end: nr_reclaimed=66',
      ' kswapd0-33    (   33) [001] ...1 12838.491407: mm_vmscan_kswapd_wake: nid=0 order=0',
      ' kswapd0-33    (   33) [001] ...1 12838.529770: mm_vmscan_kswapd_wake: nid=0 order=2',
      ' kswapd0-33    (   33) [001] ...1 12840.545737: mm_vmscan_kswapd_sleep: nid=0',
    ];
    var m = new tracing.TraceModel(lines.join('\n'), false);
    assertEquals(0, m.importErrors.length);

    assertEquals(1, m.processes['1155'].threads['1155'].sliceGroup.length);
    assertEquals(1, m.processes['33'].threads['33'].sliceGroup.length);
  });
});
