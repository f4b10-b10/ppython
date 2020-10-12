"""Test sidebar, coverage 93%"""
from textwrap import dedent
import sys

from itertools import chain
import unittest
import unittest.mock
from test.support import requires
import tkinter as tk

from idlelib.delegator import Delegator
from idlelib.editor import fixwordbreaks
from idlelib import macosx
from idlelib.percolator import Percolator
import idlelib.pyshell
from idlelib.pyshell import fix_x11_paste, PyShell, PyShellFileList
from idlelib.run import fix_scaling
import idlelib.sidebar
from idlelib.sidebar import get_end_linenumber, get_lineno


class Dummy_editwin:
    def __init__(self, text):
        self.text = text
        self.text_frame = self.text.master
        self.per = Percolator(text)
        self.undo = Delegator()
        self.per.insertfilter(self.undo)

    def setvar(self, name, value):
        pass

    def getlineno(self, index):
        return int(float(self.text.index(index)))


class LineNumbersTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()

        cls.text_frame = tk.Frame(cls.root)
        cls.text_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        cls.text_frame.rowconfigure(1, weight=1)
        cls.text_frame.columnconfigure(1, weight=1)

        cls.text = tk.Text(cls.text_frame, width=80, height=24, wrap=tk.NONE)
        cls.text.grid(row=1, column=1, sticky=tk.NSEW)

        cls.editwin = Dummy_editwin(cls.text)
        cls.editwin.vbar = tk.Scrollbar(cls.text_frame)

    @classmethod
    def tearDownClass(cls):
        cls.editwin.per.close()
        cls.root.update()
        cls.root.destroy()
        del cls.text, cls.text_frame, cls.editwin, cls.root

    def setUp(self):
        self.linenumber = idlelib.sidebar.LineNumbers(self.editwin)

        self.highlight_cfg = {"background": '#abcdef',
                              "foreground": '#123456'}
        orig_idleConf_GetHighlight = idlelib.sidebar.idleConf.GetHighlight
        def mock_idleconf_GetHighlight(theme, element):
            if element == 'linenumber':
                return self.highlight_cfg
            return orig_idleConf_GetHighlight(theme, element)
        GetHighlight_patcher = unittest.mock.patch.object(
            idlelib.sidebar.idleConf, 'GetHighlight', mock_idleconf_GetHighlight)
        GetHighlight_patcher.start()
        self.addCleanup(GetHighlight_patcher.stop)

        self.font_override = 'TkFixedFont'
        def mock_idleconf_GetFont(root, configType, section):
            return self.font_override
        GetFont_patcher = unittest.mock.patch.object(
            idlelib.sidebar.idleConf, 'GetFont', mock_idleconf_GetFont)
        GetFont_patcher.start()
        self.addCleanup(GetFont_patcher.stop)

    def tearDown(self):
        self.text.delete('1.0', 'end')

    def get_selection(self):
        return tuple(map(str, self.text.tag_ranges('sel')))

    def get_line_screen_position(self, line):
        bbox = self.linenumber.sidebar_text.bbox(f'{line}.end -1c')
        x = bbox[0] + 2
        y = bbox[1] + 2
        return x, y

    def assert_state_disabled(self):
        state = self.linenumber.sidebar_text.config()['state']
        self.assertEqual(state[-1], tk.DISABLED)

    def get_sidebar_text_contents(self):
        return self.linenumber.sidebar_text.get('1.0', tk.END)

    def assert_sidebar_n_lines(self, n_lines):
        expected = '\n'.join(chain(map(str, range(1, n_lines + 1)), ['']))
        self.assertEqual(self.get_sidebar_text_contents(), expected)

    def assert_text_equals(self, expected):
        return self.assertEqual(self.text.get('1.0', 'end'), expected)

    def test_init_empty(self):
        self.assert_sidebar_n_lines(1)

    def test_init_not_empty(self):
        self.text.insert('insert', 'foo bar\n'*3)
        self.assert_text_equals('foo bar\n'*3 + '\n')
        self.assert_sidebar_n_lines(4)

    def test_toggle_linenumbering(self):
        self.assertEqual(self.linenumber.is_shown, False)
        self.linenumber.show_sidebar()
        self.assertEqual(self.linenumber.is_shown, True)
        self.linenumber.hide_sidebar()
        self.assertEqual(self.linenumber.is_shown, False)
        self.linenumber.hide_sidebar()
        self.assertEqual(self.linenumber.is_shown, False)
        self.linenumber.show_sidebar()
        self.assertEqual(self.linenumber.is_shown, True)
        self.linenumber.show_sidebar()
        self.assertEqual(self.linenumber.is_shown, True)

    def test_insert(self):
        self.text.insert('insert', 'foobar')
        self.assert_text_equals('foobar\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

        self.text.insert('insert', '\nfoo')
        self.assert_text_equals('foobar\nfoo\n')
        self.assert_sidebar_n_lines(2)
        self.assert_state_disabled()

        self.text.insert('insert', 'hello\n'*2)
        self.assert_text_equals('foobar\nfoohello\nhello\n\n')
        self.assert_sidebar_n_lines(4)
        self.assert_state_disabled()

        self.text.insert('insert', '\nworld')
        self.assert_text_equals('foobar\nfoohello\nhello\n\nworld\n')
        self.assert_sidebar_n_lines(5)
        self.assert_state_disabled()

    def test_delete(self):
        self.text.insert('insert', 'foobar')
        self.assert_text_equals('foobar\n')
        self.text.delete('1.1', '1.3')
        self.assert_text_equals('fbar\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

        self.text.insert('insert', 'foo\n'*2)
        self.assert_text_equals('fbarfoo\nfoo\n\n')
        self.assert_sidebar_n_lines(3)
        self.assert_state_disabled()

        # Deleting up to "2.end" doesn't delete the final newline.
        self.text.delete('2.0', '2.end')
        self.assert_text_equals('fbarfoo\n\n\n')
        self.assert_sidebar_n_lines(3)
        self.assert_state_disabled()

        self.text.delete('1.3', 'end')
        self.assert_text_equals('fba\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

        # Text widgets always keep a single '\n' character at the end.
        self.text.delete('1.0', 'end')
        self.assert_text_equals('\n')
        self.assert_sidebar_n_lines(1)
        self.assert_state_disabled()

    def test_sidebar_text_width(self):
        """
        Test that linenumber text widget is always at the minimum
        width
        """
        def get_width():
            return self.linenumber.sidebar_text.config()['width'][-1]

        self.assert_sidebar_n_lines(1)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo')
        self.assert_sidebar_n_lines(1)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo\n'*8)
        self.assert_sidebar_n_lines(9)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(10)
        self.assertEqual(get_width(), 2)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(11)
        self.assertEqual(get_width(), 2)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(10)
        self.assertEqual(get_width(), 2)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(9)
        self.assertEqual(get_width(), 1)

        self.text.insert('insert', 'foo\n'*90)
        self.assert_sidebar_n_lines(99)
        self.assertEqual(get_width(), 2)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(100)
        self.assertEqual(get_width(), 3)

        self.text.insert('insert', 'foo\n')
        self.assert_sidebar_n_lines(101)
        self.assertEqual(get_width(), 3)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(100)
        self.assertEqual(get_width(), 3)

        self.text.delete('insert -1l linestart', 'insert linestart')
        self.assert_sidebar_n_lines(99)
        self.assertEqual(get_width(), 2)

        self.text.delete('50.0 -1c', 'end -1c')
        self.assert_sidebar_n_lines(49)
        self.assertEqual(get_width(), 2)

        self.text.delete('5.0 -1c', 'end -1c')
        self.assert_sidebar_n_lines(4)
        self.assertEqual(get_width(), 1)

        # Text widgets always keep a single '\n' character at the end.
        self.text.delete('1.0', 'end -1c')
        self.assert_sidebar_n_lines(1)
        self.assertEqual(get_width(), 1)

    def test_click_selection(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'one\ntwo\nthree\nfour\n')
        self.root.update()

        # Click on the second line.
        x, y = self.get_line_screen_position(2)
        self.linenumber.sidebar_text.event_generate('<Button-1>', x=x, y=y)
        self.linenumber.sidebar_text.update()
        self.root.update()

        self.assertEqual(self.get_selection(), ('2.0', '3.0'))

    def simulate_drag(self, start_line, end_line):
        start_x, start_y = self.get_line_screen_position(start_line)
        end_x, end_y = self.get_line_screen_position(end_line)

        self.linenumber.sidebar_text.event_generate('<Button-1>',
                                                    x=start_x, y=start_y)
        self.root.update()

        def lerp(a, b, steps):
            """linearly interpolate from a to b (inclusive) in equal steps"""
            last_step = steps - 1
            for i in range(steps):
                yield ((last_step - i) / last_step) * a + (i / last_step) * b

        for x, y in zip(
                map(int, lerp(start_x, end_x, steps=11)),
                map(int, lerp(start_y, end_y, steps=11)),
        ):
            self.linenumber.sidebar_text.event_generate('<B1-Motion>', x=x, y=y)
            self.root.update()

        self.linenumber.sidebar_text.event_generate('<ButtonRelease-1>',
                                                    x=end_x, y=end_y)
        self.root.update()

    def test_drag_selection_down(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'one\ntwo\nthree\nfour\nfive\n')
        self.root.update()

        # Drag from the second line to the fourth line.
        self.simulate_drag(2, 4)
        self.assertEqual(self.get_selection(), ('2.0', '5.0'))

    def test_drag_selection_up(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'one\ntwo\nthree\nfour\nfive\n')
        self.root.update()

        # Drag from the fourth line to the second line.
        self.simulate_drag(4, 2)
        self.assertEqual(self.get_selection(), ('2.0', '5.0'))

    def test_scroll(self):
        self.linenumber.show_sidebar()
        self.text.insert('1.0', 'line\n' * 100)
        self.root.update()

        # Scroll down 10 lines.
        self.text.yview_scroll(10, 'unit')
        self.root.update()
        self.assertEqual(self.text.index('@0,0'), '11.0')
        self.assertEqual(self.linenumber.sidebar_text.index('@0,0'), '11.0')

        # Generate a mouse-wheel event and make sure it scrolled up or down.
        # The meaning of the "delta" is OS-dependant, so this just checks for
        # any change.
        self.linenumber.sidebar_text.event_generate('<MouseWheel>',
                                                    x=0, y=0,
                                                    delta=10)
        self.root.update()
        self.assertNotEqual(self.text.index('@0,0'), '11.0')
        self.assertNotEqual(self.linenumber.sidebar_text.index('@0,0'), '11.0')

    def test_font(self):
        ln = self.linenumber

        orig_font = ln.sidebar_text['font']
        test_font = 'TkTextFont'
        self.assertNotEqual(orig_font, test_font)

        # Ensure line numbers aren't shown.
        ln.hide_sidebar()

        self.font_override = test_font
        # Nothing breaks when line numbers aren't shown.
        ln.update_font()

        # Activate line numbers, previous font change is immediately effective.
        ln.show_sidebar()
        self.assertEqual(ln.sidebar_text['font'], test_font)

        # Call the font update with line numbers shown, change is picked up.
        self.font_override = orig_font
        ln.update_font()
        self.assertEqual(ln.sidebar_text['font'], orig_font)

    def test_highlight_colors(self):
        ln = self.linenumber

        orig_colors = dict(self.highlight_cfg)
        test_colors = {'background': '#222222', 'foreground': '#ffff00'}

        def assert_colors_are_equal(colors):
            self.assertEqual(ln.sidebar_text['background'], colors['background'])
            self.assertEqual(ln.sidebar_text['foreground'], colors['foreground'])

        # Ensure line numbers aren't shown.
        ln.hide_sidebar()

        self.highlight_cfg = test_colors
        # Nothing breaks with inactive line numbers.
        ln.update_colors()

        # Show line numbers, previous colors change is immediately effective.
        ln.show_sidebar()
        assert_colors_are_equal(test_colors)

        # Call colors update with no change to the configured colors.
        ln.update_colors()
        assert_colors_are_equal(test_colors)

        # Call the colors update with line numbers shown, change is picked up.
        self.highlight_cfg = orig_colors
        ln.update_colors()
        assert_colors_are_equal(orig_colors)


def test_coroutine(test_method):
    def new_method(self):
        return self.run_test_coroutine(test_method(self))
    return new_method


class ShellSidebarTest(unittest.TestCase):
    root: tk.Tk = None
    shell: PyShell = None

    @classmethod
    def setUpClass(cls):
        requires('gui')

        idlelib.pyshell.use_subprocess = True

        cls.root = root = tk.Tk()
        root.withdraw()

        fix_scaling(root)
        fixwordbreaks(root)
        fix_x11_paste(root)

        cls.flist = flist = PyShellFileList(root)
        macosx.setupApp(root, flist)
        root.update_idletasks()

    @classmethod
    def tearDownClass(cls):
        if cls.shell is not None:
            cls.shell.executing = False
            cls.shell.close()
            cls.shell = None
        cls.flist = None
        cls.root.update_idletasks()
        cls.root.destroy()
        cls.root = None

    @classmethod
    def init_shell(cls):
        cls.shell = cls.flist.open_shell()
        cls.shell.pollinterval = 10
        cls.root.update()
        cls.n_preface_lines = get_lineno(cls.shell.text, 'end-1c') - 1

    @classmethod
    def reset_shell(cls):
        cls.shell.per.bottom.delete(f'{cls.n_preface_lines+1}.0', 'end-1c')
        cls.shell.shell_sidebar.update_sidebar()
        cls.root.update()

    def setUp(self):
        if self.shell is None:
            self.init_shell()
        else:
            self.reset_shell()

    def get_sidebar_lines(self):
        canvas = self.shell.shell_sidebar.canvas
        texts = list(canvas.find(tk.ALL))
        texts.sort(key=lambda text: canvas.bbox(text)[1])
        return [canvas.itemcget(text, 'text') for text in texts]

    def assert_sidebar_lines_end_with(self, expected_lines):
        self.shell.shell_sidebar.update_sidebar()
        self.assertEqual(
            self.get_sidebar_lines()[-len(expected_lines):],
            expected_lines,
        )

    def get_shell_line_y_coords(self):
        text = self.shell.text
        y_coords = []
        index = text.index("@0,0")
        while True:
            lineinfo = text.dlineinfo(index)
            if lineinfo is None:
                break
            y_coords.append(lineinfo[1])
            index = text.index(f"{index} +1line")
        return y_coords

    def get_sidebar_line_y_coords(self):
        canvas = self.shell.shell_sidebar.canvas
        texts = list(canvas.find(tk.ALL))
        texts.sort(key=lambda text: canvas.bbox(text)[1])
        return [canvas.bbox(text)[1] for text in texts]

    def assert_sidebar_lines_synced(self):
        self.assertEqual(
            self.get_sidebar_line_y_coords(),
            self.get_shell_line_y_coords(),
        )

    def do_input(self, input):
        shell = self.shell
        text = shell.text
        for line_index, line in enumerate(input.split('\n')):
            if line_index > 0:
                text.event_generate('<Key-Return>')
                text.event_generate('<KeyRelease-Return>')
            for char in line:
                text.event_generate(char)

    def run_test_coroutine(self, coroutine):
        root = self.root
        # Exceptions raised by self.assert...() need to be raised outside of
        # the after callback in order for the test harness to capture them.
        exception = None
        def after_callback():
            nonlocal exception
            try:
                interval_multiplier = next(coroutine) or 1
            except StopIteration:
                root.quit()
            except Exception as exc:
                exception = exc
                root.quit()
            else:
                root.after(100 * interval_multiplier, after_callback)
        root.after(0, after_callback)
        root.mainloop()

        if exception:
            raise exception

    def test_initial_state(self):
        sidebar_lines = self.get_sidebar_lines()
        self.assertEqual(
            sidebar_lines,
            ['   '] * (len(sidebar_lines) - 1) + ['>>>'],
        )
        self.assert_sidebar_lines_synced()

    @test_coroutine
    def test_single_empty_input(self):
        self.do_input('\n')
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '>>>'])

    @test_coroutine
    def test_single_line_command(self):
        self.do_input('1\n')
        yield 2
        self.assert_sidebar_lines_end_with(['>>>', '   ', '>>>'])

    @test_coroutine
    def test_multi_line_command(self):
        # Block statements are not indented because IDLE auto-indents.
        self.do_input(dedent('''\
            if True:
            print(1)

            '''))
        yield 2
        self.assert_sidebar_lines_end_with([
            '>>>',
            '...',
            '...',
            '...',
            '   ',
            '>>>',
        ])

    @test_coroutine
    def test_single_long_line_wraps(self):
        self.do_input('1' * 200 + '\n')
        yield 2
        self.assert_sidebar_lines_end_with(['>>>', '   ', '>>>'])
        self.assert_sidebar_lines_synced()

    @test_coroutine
    def test_squeeze_single_line_command(self):
        shell = self.shell
        text = shell.text

        self.do_input('1\n')
        yield 2
        self.assert_sidebar_lines_end_with(['>>>', '   ', '>>>'])

        line = int(shell.text.index('insert -1line').split('.')[0])
        text.mark_set('insert', f"{line}.0")
        text.event_generate('<<squeeze-current-text>>')
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '   ', '>>>'])
        self.assert_sidebar_lines_synced()

        shell.squeezer.expandingbuttons[0].expand()
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '   ', '>>>'])
        self.assert_sidebar_lines_synced()

    @test_coroutine
    def test_interrupt_recall_undo_redo(self):
        event_generate = self.shell.text.event_generate
        # Block statements are not indented because IDLE auto-indents.
        initial_sidebar_lines = self.get_sidebar_lines()

        self.do_input(dedent('''\
            if True:
            print(1)
            '''))
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '...', '...'])
        with_block_sidebar_lines = self.get_sidebar_lines()
        self.assertNotEqual(with_block_sidebar_lines, initial_sidebar_lines)

        # Control-C
        event_generate('<<interrupt-execution>>')
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '...', '...', '   ', '>>>'])

        # Recall previous via history
        event_generate('<<history-previous>>')
        event_generate('<<interrupt-execution>>')
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '...', '   ', '>>>'])

        # Recall previous via recall
        event_generate('<Key-Up>')
        event_generate('<Key-Up>')
        event_generate('<Key-Return>')
        yield 0

        event_generate('<<undo>>')
        yield 0
        self.assert_sidebar_lines_end_with(['>>>'])

        event_generate('<<redo>>')
        yield 0
        self.assert_sidebar_lines_end_with(['>>>', '...'])

        event_generate('<Key-Return>')
        event_generate('<Key-Return>')
        yield 2
        self.assert_sidebar_lines_end_with(
            ['>>>', '...', '...', '...', '   ', '>>>']
        )

    def test_font(self):
        sidebar = self.shell.shell_sidebar

        test_font = 'TkTextFont'

        def mock_idleconf_GetFont(root, configType, section):
            return test_font
        GetFont_patcher = unittest.mock.patch.object(
            idlelib.sidebar.idleConf, 'GetFont', mock_idleconf_GetFont)
        GetFont_patcher.start()
        def cleanup():
            GetFont_patcher.stop()
            sidebar.update_font()
        self.addCleanup(cleanup)

        def get_sidebar_font():
            canvas = sidebar.canvas
            texts = list(canvas.find(tk.ALL))
            fonts = {canvas.itemcget(text, 'font') for text in texts}
            self.assertEqual(len(fonts), 1)
            return next(iter(fonts))

        self.assertNotEqual(get_sidebar_font(), test_font)
        sidebar.update_font()
        self.assertEqual(get_sidebar_font(), test_font)

    def test_highlight_colors(self):
        sidebar = self.shell.shell_sidebar

        test_colors = {"background": '#abcdef', "foreground": '#123456'}

        orig_idleConf_GetHighlight = idlelib.sidebar.idleConf.GetHighlight
        def mock_idleconf_GetHighlight(theme, element):
            if element in ['linenumber', 'console']:
                return test_colors
            return orig_idleConf_GetHighlight(theme, element)
        GetHighlight_patcher = unittest.mock.patch.object(
            idlelib.sidebar.idleConf, 'GetHighlight',
            mock_idleconf_GetHighlight)
        GetHighlight_patcher.start()
        def cleanup():
            GetHighlight_patcher.stop()
            sidebar.update_colors()
        self.addCleanup(cleanup)

        def get_sidebar_colors():
            canvas = sidebar.canvas
            texts = list(canvas.find(tk.ALL))
            fgs = {canvas.itemcget(text, 'fill') for text in texts}
            self.assertEqual(len(fgs), 1)
            fg = next(iter(fgs))
            bg = canvas.cget('background')
            return {"background": bg, "foreground": fg}

        self.assertNotEqual(get_sidebar_colors(), test_colors)
        sidebar.update_colors()
        self.assertEqual(get_sidebar_colors(), test_colors)

    @test_coroutine
    def test_mousewheel(self):
        sidebar = self.shell.shell_sidebar
        text = self.shell.text

        # Press Return 50 times to get the shell screen to scroll down.
        for _i in range(50):
            self.do_input('\n')
        yield 0
        self.assertGreater(get_lineno(text, '@0,0'), 1)

        last_lineno = get_end_linenumber(text)
        self.assertIsNotNone(text.dlineinfo(text.index(f'{last_lineno}.0')))

        # Scroll up using the <MouseWheel> event with a positive delta.
        delta = -1 if sys.platform == 'darwin' else 120
        sidebar.canvas.event_generate('<MouseWheel>', x=0, y=0, delta=delta)
        yield 0
        self.assertIsNone(text.dlineinfo(text.index(f'{last_lineno}.0')))

        # Scroll back down using the <Button-5> event.
        sidebar.canvas.event_generate('<Button-5>', x=0, y=0)
        yield 0
        self.assertIsNotNone(text.dlineinfo(text.index(f'{last_lineno}.0')))


if __name__ == '__main__':
    unittest.main(verbosity=2)
