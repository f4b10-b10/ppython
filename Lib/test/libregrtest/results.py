import sys
from test.support import TestStats

from .runtests import RunTests
from .result import State, TestResult
from .utils import (
    StrPath, TestName, TestTuple, TestList, FilterDict,
    printlist, count, format_duration)


# Python uses exit code 1 when an exception is not catched
# argparse.ArgumentParser.error() uses exit code 2
EXITCODE_BAD_TEST = 2
EXITCODE_ENV_CHANGED = 3
EXITCODE_NO_TESTS_RAN = 4
EXITCODE_RERUN_FAIL = 5
EXITCODE_INTERRUPTED = 130   # 128 + signal.SIGINT=2


class TestResults:
    def __init__(self):
        self.bad: TestList = []
        self.good: TestList = []
        self.rerun_bad: TestList = []
        self.skipped: TestList = []
        self.resource_denied: TestList = []
        self.env_changed: TestList = []
        self.run_no_tests: TestList = []
        self.rerun: TestList = []
        self.rerun_results: list[TestResult] = []

        self.interrupted: bool = False
        self.test_times: list[tuple[float, TestName]] = []
        self.stats = TestStats()
        # used by --junit-xml
        self.testsuite_xml: list[str] = []

    def is_all_good(self):
        return (not self.bad
                and not self.skipped
                and not self.interrupted)

    def get_executed(self):
        return (set(self.good) | set(self.bad) | set(self.skipped)
                | set(self.resource_denied) | set(self.env_changed)
                | set(self.run_no_tests))

    def no_tests_run(self):
        return not any((self.good, self.bad, self.skipped, self.interrupted,
                        self.env_changed))

    def get_state(self, fail_env_changed):
        state = []
        if self.bad:
            state.append("FAILURE")
        elif fail_env_changed and self.env_changed:
            state.append("ENV CHANGED")
        elif self.no_tests_run():
            state.append("NO TESTS RAN")

        if self.interrupted:
            state.append("INTERRUPTED")
        if not state:
            state.append("SUCCESS")

        return ', '.join(state)

    def get_exitcode(self, fail_env_changed, fail_rerun):
        exitcode = 0
        if self.bad:
            exitcode = EXITCODE_BAD_TEST
        elif self.interrupted:
            exitcode = EXITCODE_INTERRUPTED
        elif fail_env_changed and self.env_changed:
            exitcode = EXITCODE_ENV_CHANGED
        elif self.no_tests_run():
            exitcode = EXITCODE_NO_TESTS_RAN
        elif fail_rerun and self.rerun:
            exitcode = EXITCODE_RERUN_FAIL
        return exitcode

    def accumulate_result(self, result: TestResult, runtests: RunTests):
        test_name = result.test_name
        rerun = runtests.rerun
        fail_env_changed = runtests.fail_env_changed

        match result.state:
            case State.PASSED:
                self.good.append(test_name)
            case State.ENV_CHANGED:
                self.env_changed.append(test_name)
                self.rerun_results.append(result)
            case State.SKIPPED:
                self.skipped.append(test_name)
            case State.RESOURCE_DENIED:
                self.resource_denied.append(test_name)
            case State.INTERRUPTED:
                self.interrupted = True
            case State.DID_NOT_RUN:
                self.run_no_tests.append(test_name)
            case _:
                if result.is_failed(fail_env_changed):
                    self.bad.append(test_name)
                    self.rerun_results.append(result)
                else:
                    raise ValueError(f"invalid test state: {result.state!r}")

        if result.has_meaningful_duration() and not rerun:
            self.test_times.append((result.duration, test_name))
        if result.stats is not None:
            self.stats.accumulate(result.stats)
        if rerun:
            self.rerun.append(test_name)

        xml_data = result.xml_data
        if xml_data:
            self.add_junit(xml_data)

    def need_rerun(self):
        return bool(self.rerun_results)

    def prepare_rerun(self) -> tuple[TestTuple, FilterDict]:
        tests: TestList = []
        match_tests_dict = {}
        for result in self.rerun_results:
            tests.append(result.test_name)

            match_tests = result.get_rerun_match_tests()
            # ignore empty match list
            if match_tests:
                match_tests_dict[result.test_name] = match_tests

        # Clear previously failed tests
        self.rerun_bad.extend(self.bad)
        self.bad.clear()
        self.env_changed.clear()
        self.rerun_results.clear()

        return (tuple(tests), match_tests_dict)

    def add_junit(self, xml_data: list[str]):
        import xml.etree.ElementTree as ET
        for e in xml_data:
            try:
                self.testsuite_xml.append(ET.fromstring(e))
            except ET.ParseError:
                print(xml_data, file=sys.__stderr__)
                raise

    def write_junit(self, filename: StrPath):
        if not self.testsuite_xml:
            # Don't create empty XML file
            return

        import xml.etree.ElementTree as ET
        root = ET.Element("testsuites")

        # Manually count the totals for the overall summary
        totals = {'tests': 0, 'errors': 0, 'failures': 0}
        for suite in self.testsuite_xml:
            root.append(suite)
            for k in totals:
                try:
                    totals[k] += int(suite.get(k, 0))
                except ValueError:
                    pass

        for k, v in totals.items():
            root.set(k, str(v))

        with open(filename, 'wb') as f:
            for s in ET.tostringlist(root):
                f.write(s)

    def display_result(self, tests: TestTuple, quiet: bool, print_slowest: bool):
        omitted = set(tests) - self.get_executed()
        if omitted:
            print()
            print(count(len(omitted), "test"), "omitted:")
            printlist(omitted)

        if print_slowest:
            self.test_times.sort(reverse=True)
            print()
            print("10 slowest tests:")
            for test_time, test in self.test_times[:10]:
                print("- %s: %s" % (test, format_duration(test_time)))

        all_tests = [
            (self.bad, "test", "{} failed:"),
            (self.env_changed, "test", "{} altered the execution environment (env changed):"),
        ]
        if not quiet:
            all_tests.append((self.skipped, "test", "{} skipped:"))
            all_tests.append((self.resource_denied, "test", "{} skipped (resource denied):"))
        all_tests.append((self.rerun, "re-run test", "{}:"))
        all_tests.append((self.run_no_tests, "test", "{} run no tests:"))

        for tests_list, count_text, title_format in all_tests:
            if tests_list:
                print()
                count_text = count(len(tests_list), count_text)
                print(title_format.format(count_text))
                printlist(tests_list)

        if self.good and not quiet:
            print()
            text = count(len(self.good), "test")
            text = f"{text} OK."
            if (self.is_all_good() and len(self.good) > 1):
                text = f"All {text}"
            print(text)

        if self.interrupted:
            print()
            print("Test suite interrupted by signal SIGINT.")

    def display_summary(self, first_runtests: RunTests, filtered: bool):
        # Total tests
        stats = self.stats
        text = f'run={stats.tests_run:,}'
        if filtered:
            text = f"{text} (filtered)"
        report = [text]
        if stats.failures:
            report.append(f'failures={stats.failures:,}')
        if stats.skipped:
            report.append(f'skipped={stats.skipped:,}')
        print(f"Total tests: {' '.join(report)}")

        # Total test files
        all_tests = [self.good, self.bad, self.rerun,
                     self.skipped,
                     self.env_changed, self.run_no_tests]
        run = sum(map(len, all_tests))
        text = f'run={run}'
        if not first_runtests.forever:
            ntest = len(first_runtests.tests)
            text = f"{text}/{ntest}"
        if filtered:
            text = f"{text} (filtered)"
        report = [text]
        for name, tests in (
            ('failed', self.bad),
            ('env_changed', self.env_changed),
            ('skipped', self.skipped),
            ('resource_denied', self.resource_denied),
            ('rerun', self.rerun),
            ('run_no_tests', self.run_no_tests),
        ):
            if tests:
                report.append(f'{name}={len(tests)}')
        print(f"Total test files: {' '.join(report)}")
