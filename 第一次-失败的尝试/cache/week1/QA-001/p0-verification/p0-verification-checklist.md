# P0 Defect Verification Checklist
**Version**: 1.0.0  
**Date**: 2025-08-25  
**Deployment**: 22:00 Tonight  
**Verification Window**: 23:30-00:30

## Critical P0 Defects for Verification

### DEFECT-001: Memory Leak in Frame Aggregator
**Severity**: P0 - Critical  
**Component**: libtmuxcore frame aggregator  
**Original Issue**: 15MB/hour memory leak during continuous operation

#### Pre-Verification Steps
- [ ] Stop all running tmux sessions
- [ ] Clear system memory caches: `sudo purge` (macOS)
- [ ] Record baseline memory usage
- [ ] Enable memory profiling tools
- [ ] Start monitoring with `leaks` and `vmmap`

#### Verification Criteria
1. **Memory Stability Test**
   - Run frame aggregator for 30 minutes under load
   - Memory growth must be < 100KB total
   - No leaks reported by valgrind/leaks tool
   
2. **Stress Test**
   - Process 10,000 frames in rapid succession
   - Memory must return to baseline ±5% after completion
   - No orphaned allocations

3. **Long-Running Test**
   - 2-hour continuous operation
   - Memory growth < 500KB total
   - RSS memory stable within 2% variance

#### Pass/Fail Conditions
**PASS**: All three tests complete with criteria met  
**FAIL**: Any single criterion not met  
**CRITICAL FAIL**: Memory growth > 1MB in any test

#### Rollback Procedure
```bash
# If verification fails:
1. git revert HEAD~1  # Revert the fix commit
2. ./rollback-deployment.sh DEFECT-001
3. Alert on-call engineer immediately
4. Generate memory dump for analysis
5. Restore previous libtmuxcore.so from backup
```

---

### DEFECT-002: Race Condition in Callback System
**Severity**: P0 - Critical  
**Component**: FFI callback dispatcher  
**Original Issue**: Segfault under concurrent callback execution

#### Pre-Verification Steps
- [ ] Enable thread sanitizer (TSAN)
- [ ] Set up concurrent load generator
- [ ] Configure core dump collection
- [ ] Start dtrace/instruments monitoring
- [ ] Clear all logs and crash reports

#### Verification Criteria
1. **Thread Safety Test**
   - 100 concurrent threads calling callbacks
   - Zero segfaults or crashes
   - No data races detected by TSAN
   
2. **Stress Concurrency Test**
   - 1000 callbacks/second for 10 minutes
   - All callbacks complete successfully
   - No deadlocks or livelocks
   
3. **Order Preservation Test**
   - Verify callback ordering maintained
   - Sequence numbers match expected
   - No out-of-order execution

#### Pass/Fail Conditions
**PASS**: Zero crashes, zero races, correct ordering  
**FAIL**: Any crash, race condition, or ordering issue  
**CRITICAL FAIL**: Segfault or deadlock detected

#### Rollback Procedure
```bash
# If verification fails:
1. Kill all tmux processes immediately
2. git revert HEAD~1  # Revert the fix commit
3. ./rollback-deployment.sh DEFECT-002
4. Collect all core dumps and crash logs
5. Restore thread-safe version from backup
6. Page on-call engineer with crash details
```

---

## Integration Coverage Verification (INTG-001)

### Target Coverage: 75%+

#### Pre-Verification Steps
- [ ] Clean all previous coverage data
- [ ] Rebuild with coverage flags enabled
- [ ] Verify gcov/lcov tools available
- [ ] Baseline coverage recorded

#### Verification Criteria
1. **Line Coverage**: ≥ 75%
2. **Branch Coverage**: ≥ 70%
3. **Function Coverage**: ≥ 80%
4. **Critical Path Coverage**: 100%

#### Pass/Fail Conditions
**PASS**: All coverage targets met  
**FAIL**: Any coverage below target  
**WARNING**: Coverage 70-74% (conditional pass)

---

## Go/No-Go Decision Matrix

| Test Component | Weight | Pass | Fail | Action |
|----------------|--------|------|------|--------|
| DEFECT-001 Memory | 40% | ✓ | ✗ | NO-GO: Rollback |
| DEFECT-002 Race | 40% | ✓ | ✗ | NO-GO: Rollback |
| Coverage Target | 10% | ✓ | ✗ | WARNING: Proceed with caution |
| Regression Suite | 10% | ✓ | ✗ | NO-GO: Investigate |

**Decision Rules**:
- ALL P0 defects must PASS for GO
- Coverage can be WARNING for conditional GO
- Any CRITICAL FAIL = immediate NO-GO
- 2+ regular FAILs = NO-GO

---

## Verification Timeline

### Tonight (Day 1)
- **22:00**: Deployment begins
- **22:30**: Initial smoke tests
- **23:00**: Begin P0 verification
- **23:30**: DEFECT-001 memory verification
- **00:00**: DEFECT-002 race condition verification
- **00:30**: Initial report

### Tomorrow (Day 2)
- **09:00**: Coverage validation begins
- **10:00**: Full regression suite
- **11:00**: Final verification complete
- **11:30**: Go/No-Go decision
- **12:00**: Production deployment or rollback

---

## Emergency Contacts

- **On-Call Engineer**: Check PagerDuty
- **QA Lead**: qa-test-lead@ghostty-core:0
- **Project Manager**: tmux-orchestrator:0
- **Rollback Hotline**: ./emergency-rollback.sh

---

## Audit Trail Requirements

All verification steps must generate:
1. Timestamped logs in `logs/[timestamp]/`
2. Screenshots/recordings of failures
3. Memory/CPU/thread metrics
4. Coverage reports with diffs
5. Sign-off documentation

**Verification Sign-off Required By**:
- [ ] QA Test Lead
- [ ] Integration Developer
- [ ] Project Manager