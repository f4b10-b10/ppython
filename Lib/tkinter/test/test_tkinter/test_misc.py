import unittest
import tkinter
from test import support
from tkinter.test.support import AbstractTkTest

support.requires('gui')

class MiscTest(AbstractTkTest, unittest.TestCase):

    def test_all(self):
        self.assertIn("Widget", tkinter.__all__)
        # Check that variables from tkinter.constants are also in tkinter.__all__
        self.assertIn("CASCADE", tkinter.__all__)
        self.assertIsNotNone(tkinter.CASCADE)
        # Check that sys, re, and constants are not in tkinter.__all__
        self.assertNotIn("re", tkinter.__all__)
        self.assertNotIn("sys", tkinter.__all__)
        self.assertNotIn("constants", tkinter.__all__)
        # Check that an underscored functions is not in tkinter.__all__
        self.assertNotIn("_tkerror", tkinter.__all__)
        # Check that wantobjects is not in tkinter.__all__
        self.assertNotIn("wantobjects", tkinter.__all__)

    def test_repr(self):
        t = tkinter.Toplevel(self.root, name='top')
        f = tkinter.Frame(t, name='child')
        self.assertEqual(repr(f), '<tkinter.Frame object .top.child>')

    def test_generated_names(self):
        t = tkinter.Toplevel(self.root)
        f = tkinter.Frame(t)
        f2 = tkinter.Frame(t)
        b = tkinter.Button(f2)
        for name in str(b).split('.'):
            self.assertFalse(name.isidentifier(), msg=repr(name))

    def test_tk_setPalette(self):
        root = self.root
        root.tk_setPalette('black')
        self.assertEqual(root['background'], 'black')
        root.tk_setPalette('white')
        self.assertEqual(root['background'], 'white')
        self.assertRaisesRegex(tkinter.TclError,
                '^unknown color name "spam"$',
                root.tk_setPalette, 'spam')

        root.tk_setPalette(background='black')
        self.assertEqual(root['background'], 'black')
        root.tk_setPalette(background='blue', highlightColor='yellow')
        self.assertEqual(root['background'], 'blue')
        self.assertEqual(root['highlightcolor'], 'yellow')
        root.tk_setPalette(background='yellow', highlightColor='blue')
        self.assertEqual(root['background'], 'yellow')
        self.assertEqual(root['highlightcolor'], 'blue')
        self.assertRaisesRegex(tkinter.TclError,
                '^unknown color name "spam"$',
                root.tk_setPalette, background='spam')
        self.assertRaisesRegex(tkinter.TclError,
                '^must specify a background color$',
                root.tk_setPalette, spam='white')
        self.assertRaisesRegex(tkinter.TclError,
                '^must specify a background color$',
                root.tk_setPalette, highlightColor='blue')

    def test_after(self):
        root = self.root

        def callback(start=0, step=1):
            nonlocal count
            count = start + step

        # Without function, sleeps for ms.
        self.assertIsNone(root.after(1))

        # Set up with callback with no args.
        count = 0
        timer1 = root.after(0, callback)
        self.assertIn(timer1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', timer1))
        root.update()  # Process all pending events.
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

        # Set up with callback with args.
        count = 0
        timer1 = root.after(0, callback, 42, 11)
        root.update()  # Process all pending events.
        self.assertEqual(count, 53)

        # Cancel before called.
        timer1 = root.after(1000, callback)
        self.assertIn(timer1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', timer1))
        root.after_cancel(timer1)  # Cancel this event.
        self.assertEqual(count, 53)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

    def test_after_idle(self):
        root = self.root

        def callback(start=0, step=1):
            nonlocal count
            count = start + step

        # Set up with callback with no args.
        count = 0
        idle1 = root.after_idle(callback)
        self.assertIn(idle1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', idle1))
        root.update_idletasks()  # Process all pending events.
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

        # Set up with callback with args.
        count = 0
        idle1 = root.after_idle(callback, 42, 11)
        root.update_idletasks()  # Process all pending events.
        self.assertEqual(count, 53)

        # Cancel before called.
        idle1 = root.after_idle(callback)
        self.assertIn(idle1, root.tk.call('after', 'info'))
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', idle1))
        root.after_cancel(idle1)  # Cancel this event.
        self.assertEqual(count, 53)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)

    def test_after_cancel(self):
        root = self.root

        def callback():
            nonlocal count
            count += 1

        timer1 = root.after(5000, callback)
        idle1 = root.after_idle(callback)

        # No value for id raises a ValueError.
        with self.assertRaises(ValueError):
            root.after_cancel(None)

        # Cancel timer event.
        count = 0
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', timer1))
        root.tk.call(script)
        self.assertEqual(count, 1)
        root.after_cancel(timer1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call('after', 'info', timer1)

        # Cancel same event - nothing happens.
        root.after_cancel(timer1)

        # Cancel idle event.
        count = 0
        (script, _) = root.tk.splitlist(root.tk.call('after', 'info', idle1))
        root.tk.call(script)
        self.assertEqual(count, 1)
        root.after_cancel(idle1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call(script)
        self.assertEqual(count, 1)
        with self.assertRaises(tkinter.TclError):
            root.tk.call('after', 'info', idle1)

    def test_clipboard(self):
        root = self.root
        root.clipboard_clear()
        root.clipboard_append('Ùñî')
        self.assertEqual(root.clipboard_get(), 'Ùñî')
        root.clipboard_append('çōđě')
        self.assertEqual(root.clipboard_get(), 'Ùñîçōđě')
        root.clipboard_clear()
        with self.assertRaises(tkinter.TclError):
            root.clipboard_get()

    def test_clipboard_astral(self):
        root = self.root
        root.clipboard_clear()
        root.clipboard_append('𝔘𝔫𝔦')
        self.assertEqual(root.clipboard_get(), '𝔘𝔫𝔦')
        root.clipboard_append('𝔠𝔬𝔡𝔢')
        self.assertEqual(root.clipboard_get(), '𝔘𝔫𝔦𝔠𝔬𝔡𝔢')
        root.clipboard_clear()
        with self.assertRaises(tkinter.TclError):
            root.clipboard_get()

    def test_winfo_rgb(self):
        root = self.root
        rgb = root.winfo_rgb

        # Color name.
        self.assertEqual(rgb('red'), (65535, 0, 0))
        self.assertEqual(rgb('dark slate blue'), (18504, 15677, 35723))
        # #RGB - extends each 4-bit hex value to be 16-bit.
        self.assertEqual(rgb('#4CE'), (0x4444, 0xCCCC, 0xEEEE))
        # #RRGGBB - extends each 8-bit hex value to be 16-bit.
        self.assertEqual(rgb('#483d8b'), (0x4848, 0x3d3d, 0x8b8b))
        # #RRRGGGBBB - uses 2 highest order bits for each color.
        self.assertEqual(rgb('#123456789'), (0x1212, 0x4545, 0x7878))
        # #RRRRGGGGBBBB - uses 2 highest order bits for each color.
        self.assertEqual(rgb('#123456789abc'), (0x1212, 0x5656, 0x9a9a))
        # Invalid string.
        with self.assertRaises(tkinter.TclError):
            rgb('#123456789a')
        # RGB triplet is invalid input.
        with self.assertRaises(tkinter.TclError):
            rgb((111, 78, 55))


tests_gui = (MiscTest, )

if __name__ == "__main__":
    support.run_unittest(*tests_gui)
