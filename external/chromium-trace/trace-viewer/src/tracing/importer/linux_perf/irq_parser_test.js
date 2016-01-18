// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

'use strict';

base.require('tracing.test_utils');
base.require('tracing.importer.linux_perf_importer');

base.unittest.testSuite('tracing.importer.linux_perf.irq_parser', function() { // @suppress longLineCheck
  test('irqImport', function() {
    var lines = [
      ' kworker/u4:1-31907 (31907) [001] d.h3 14063.748288: ' +
        'irq_handler_entry: irq=27 name=arch_timer',
      ' kworker/u4:1-31907 (31907) [001] dNh3 14063.748384: ' +
        'irq_handler_exit: irq=27 ret=handled',
      ' kworker/u4:2-31908 (31908) [000] ..s3 14063.477231: ' +
        'softirq_entry: vec=9 [action=RCU]',
      ' kworker/u4:2-31908 (31908) [000] ..s3 14063.477246: ' +
        'softirq_exit: vec=9 [action=RCU]',
    ];
    var m = new tracing.TraceModel(lines.join('\n'), false);
    assertEquals(0, m.importErrors.length);

    var threads = m.getAllThreads();
    assertEquals(2, threads.length);

    var threads = m.findAllThreadsNamed('irqs cpu 1');
    assertEquals(1, threads.length);
    assertEquals(1, threads[0].sliceGroup.length);

    var threads = m.findAllThreadsNamed('softirq cpu 0');
    assertEquals(1, threads.length);
    assertEquals(1, threads[0].sliceGroup.length);
  });
});
