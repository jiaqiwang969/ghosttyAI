# PM Validation Checklist

## Code Promotion from Cache to Src

### Pre-Validation Checks

#### 1. Test-First Verification
- [ ] Test file exists in cache/week1/[ROLE]/tests/
- [ ] Test file timestamp BEFORE implementation file
- [ ] Test file includes all acceptance criteria
- [ ] Tests were initially failing (check git history)

#### 2. Coverage Analysis
```bash
# Run coverage check
cd cache/week1/[ROLE]/
gcov *.c
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```
- [ ] New code coverage >80%
- [ ] Modified code coverage >70%
- [ ] Integration points coverage >90%
- [ ] Coverage report archived

#### 3. Test Execution
```bash
# Run all tests
make -C cache/week1/[ROLE]/tests test
```
- [ ] All unit tests passing
- [ ] Integration tests passing
- [ ] Performance benchmarks met
- [ ] No memory leaks (valgrind clean)

#### 4. Code Quality Review
- [ ] No compiler warnings
- [ ] Consistent code style
- [ ] Error handling complete
- [ ] Comments and documentation present
- [ ] No TODO/FIXME in production code

#### 5. Handoff Documentation
- [ ] README.md present in handoffs/
- [ ] API documentation complete
- [ ] Usage examples provided
- [ ] Known limitations documented
- [ ] Dependencies clearly stated

#### 6. Performance Validation
- [ ] Baseline metrics compared
- [ ] No performance regression
- [ ] Callback frequency ≤60Hz
- [ ] Memory increase <10%
- [ ] Routing overhead <2%

### Validation Decision

**APPROVED** ✅ / **REJECTED** ❌

If APPROVED:
1. Move code from cache/week1/[ROLE]/handoffs/ to cache/validated/ready_for_src/
2. Tag with version: `validated-[ROLE]-T[XXX]-v1.0.0`
3. Update dependency graph
4. Notify dependent teams

If REJECTED:
1. Document rejection reasons
2. Return to engineer with specific requirements
3. Reset task status to IMPLEMENTING
4. Schedule re-review

### Validation Record

```
Validator: PM
Date: [DATE]
Time: [TIME]
Task: T-XXX
Engineer: [ROLE]
Decision: [APPROVED/REJECTED]
Version Tag: [if approved]
Notes: [detailed feedback]
```

### Post-Validation Actions

- [ ] Update cache/shared/dependencies/dependency_graph.json
- [ ] Update cache/shared/interfaces/versions.json
- [ ] Notify dependent engineers of availability
- [ ] Archive test results
- [ ] Update project metrics

### Integration Checklist

Before moving to src/:
- [ ] All dependencies validated
- [ ] Interface versions locked
- [ ] Integration tests with dependencies passing
- [ ] Architect consulted (if interface changes)
- [ ] DevOps consulted (if build changes)
- [ ] Final PM approval granted

---

## Daily Validation Schedule

| Time | Action |
|------|--------|
| 10:00 | Morning validation round |
| 14:00 | Afternoon validation round |
| 17:00 | End-of-day validation |

## Escalation for Failed Validation

1. **First Rejection**: Return to engineer with specific fixes
2. **Second Rejection**: Pair review with PM
3. **Third Rejection**: Escalate to Architect for design review

---

**Remember**: Quality is not negotiable. Every validation protects the project from technical debt.